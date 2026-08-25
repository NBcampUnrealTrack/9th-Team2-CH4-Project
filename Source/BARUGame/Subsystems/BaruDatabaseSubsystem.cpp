#include "Subsystems/BaruDatabaseSubsystem.h"
#include "Async/Async.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "BaruLog.h"

UBaruDatabaseSubsystem::UBaruDatabaseSubsystem()
{
}

void UBaruDatabaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Dedicated Server의 Saved/SaveGames 폴더에 SQLite .db 파일 경로 설정
    const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    IFileManager::Get().MakeDirectory(*SaveDir, true);
    DbPath = SaveDir / TEXT("BaruGameData.db");

    InitializeTables();
    BARU_LOG(LogBaruBackend, Log, TEXT("BaruDatabaseSubsystem Initialized. DB Path: %s"), *DbPath);
}

void UBaruDatabaseSubsystem::Deinitialize()
{
    BARU_LOG(LogBaruBackend, Log, TEXT("BaruDatabaseSubsystem Deinitialized."));
    Super::Deinitialize();
}

void UBaruDatabaseSubsystem::InitializeTables()
{
    FScopeLock ScopeLock(&DbLock);

    FSQLiteDatabase Db;
    if (Db.Open(*DbPath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
    {
        // 1. 유저 계정 및 전적 테이블 생성
        const FString CreateUsersTable = TEXT(
            "CREATE TABLE IF NOT EXISTS Users ("
            "    PlayerId TEXT PRIMARY KEY,"
            "    Nickname TEXT DEFAULT 'Operative',"
            "    Gold INTEGER DEFAULT 0,"
            "    Survivals INTEGER DEFAULT 0,"
            "    Deaths INTEGER DEFAULT 0"
            ");"
        );
        Db.Execute(*CreateUsersTable);

        // 2. 은닉처(Stash) 영구 아이템 보관 테이블 생성
        const FString CreateStashTable = TEXT(
            "CREATE TABLE IF NOT EXISTS Stash_Items ("
            "    ItemInstanceId TEXT PRIMARY KEY,"
            "    PlayerId TEXT NOT NULL,"
            "    ItemId TEXT NOT NULL,"
            "    Quantity INTEGER DEFAULT 1,"
            "    Durability REAL DEFAULT 100.0,"
            "    FOREIGN KEY(PlayerId) REFERENCES Users(PlayerId)"
            ");"
        );
        Db.Execute(*CreateStashTable);

        // 3. 정산 이력 로그 테이블 생성
        const FString CreateLogsTable = TEXT(
            "CREATE TABLE IF NOT EXISTS Settlement_Logs ("
            "    LogId TEXT PRIMARY KEY,"
            "    PlayerId TEXT NOT NULL,"
            "    IsExtracted INTEGER NOT NULL,"
            "    EarnedGold INTEGER DEFAULT 0,"
            "    CreatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
        );
        Db.Execute(*CreateLogsTable);

        Db.Close();
    }
    else
    {
        BARU_LOG(LogBaruBackend, Error, TEXT("Failed to open/create database during InitializeTables: %s"), *DbPath);
    }
}

void UBaruDatabaseSubsystem::SaveSettlementAsync(const FBaruSettlementContext& Context, FOnBaruSettlementCompleted OnCompleted)
{
    // 백그라운드 워커 스레드로 I/O 위임
    Async(EAsyncExecution::Thread, [this, Context, OnCompleted]()
    {
        const bool bSuccess = ExecuteSettlementTransaction_Internal(Context);
        
        Async(EAsyncExecution::TaskGraphMainThread, [Context, bSuccess, OnCompleted]()
        {
            if (OnCompleted.IsBound())
            {
                OnCompleted.Execute(Context.PlayerId, bSuccess);
            }
        });
    });
}

void UBaruDatabaseSubsystem::LoadPlayerProfileAsync(const FString& PlayerId, FOnBaruProfileLoaded OnCompleted)
{
    Async(EAsyncExecution::Thread, [this, PlayerId, OnCompleted]()
    {
        FBaruPlayerProfileData Profile;
        const bool bSuccess = ExecuteLoadProfile_Internal(PlayerId, Profile);
        
        Async(EAsyncExecution::TaskGraphMainThread, [bSuccess, Profile, OnCompleted]()
        {
            if (OnCompleted.IsBound())
            {
                OnCompleted.Execute(bSuccess, Profile);
            }
        });
    });
}

bool UBaruDatabaseSubsystem::ExecuteSettlementTransaction_Internal(const FBaruSettlementContext& Context)
{
    FScopeLock ScopeLock(&DbLock);

    FSQLiteDatabase Db;
    if (!Db.Open(*DbPath, ESQLiteDatabaseOpenMode::ReadWrite))
    {
        BARU_LOG(LogBaruBackend, Error, TEXT("DB Open Failed for Settlement: %s"), *DbPath);
        return false;
    }

    // 1. ACID 트랜잭션 시작
    if (!Db.Execute(TEXT("BEGIN TRANSACTION;")))
    {
        BARU_LOG(LogBaruBackend, Error, TEXT("Failed to begin transaction."));
        Db.Close();
        return false;
    }

    bool bTransactionSucceeded = true;

    if (Context.bIsExtracted) // [탈출 성공]
    {
        // 2-A. 유저 골드 가산 및 생존 횟수 증가 (UPSERT)
        const FString UpdateUserQuery = FString::Printf(
            TEXT("INSERT INTO Users (PlayerId, Nickname, Gold, Survivals, Deaths) "
                 "VALUES ('%s', 'Operative', %d, 1, 0) "
                 "ON CONFLICT(PlayerId) DO UPDATE SET "
                 "Gold = Gold + %d, "
                 "Survivals = Survivals + 1;"),
            *Context.PlayerId, Context.TotalEarnedGold, Context.TotalEarnedGold
        );

        if (!Db.Execute(*UpdateUserQuery))
        {
            BARU_LOG(LogBaruBackend, Error, TEXT("User update query failed: %s"), *UpdateUserQuery);
            bTransactionSucceeded = false;
        }

        // 2-B. 획득 수집품을 은닉처(Stash_Items)에 일괄 INSERT
        if (bTransactionSucceeded)
        {
            for (const FBaruSettlementItemData& Loot : Context.AcquiredLoots)
            {
                const FString ItemInstanceId = FGuid::NewGuid().ToString();
                const FString InsertItemQuery = FString::Printf(
                    TEXT("INSERT INTO Stash_Items (ItemInstanceId, PlayerId, ItemId, Quantity, Durability) "
                         "VALUES ('%s', '%s', '%s', %d, 100.0);"),
                    *ItemInstanceId, *Context.PlayerId, *Loot.ItemId, Loot.Quantity
                );

                if (!Db.Execute(*InsertItemQuery))
                {
                    BARU_LOG(LogBaruBackend, Error, TEXT("Stash item insert failed: %s"), *InsertItemQuery);
                    bTransactionSucceeded = false;
                    break;
                }
            }
        }
    }
    else // [사망 / 탈출 실패]
    {
        // 2-C. 사망 횟수 증가
        const FString UpdateDeathQuery = FString::Printf(
            TEXT("INSERT INTO Users (PlayerId, Nickname, Gold, Survivals, Deaths) "
                 "VALUES ('%s', 'Operative', 0, 0, 1) "
                 "ON CONFLICT(PlayerId) DO UPDATE SET "
                 "Deaths = Deaths + 1;"),
            *Context.PlayerId
        );

        if (!Db.Execute(*UpdateDeathQuery))
        {
            BARU_LOG(LogBaruBackend, Error, TEXT("Death update query failed: %s"), *UpdateDeathQuery);
            bTransactionSucceeded = false;
        }
    }

    // 2-D. 정산 로그 기록
    if (bTransactionSucceeded)
    {
        const FString LogId = FGuid::NewGuid().ToString();
        const FString InsertLogQuery = FString::Printf(
            TEXT("INSERT INTO Settlement_Logs (LogId, PlayerId, IsExtracted, EarnedGold) "
                 "VALUES ('%s', '%s', %d, %d);"),
            *LogId, *Context.PlayerId, Context.bIsExtracted ? 1 : 0, Context.TotalEarnedGold
        );
        Db.Execute(*InsertLogQuery);
    }

    // 3. 트랜잭션 종료 (COMMIT 또는 ROLLBACK)
    if (bTransactionSucceeded)
    {
        Db.Execute(TEXT("COMMIT;"));
        BARU_LOG(LogBaruBackend, Log, TEXT("Settlement transaction committed successfully for Player [%s]."), *Context.PlayerId);
    }
    else
    {
        Db.Execute(TEXT("ROLLBACK;"));
        BARU_LOG(LogBaruBackend, Error, TEXT("Settlement transaction rolled back for Player [%s]."), *Context.PlayerId);
    }

    Db.Close();
    return bTransactionSucceeded;
}

bool UBaruDatabaseSubsystem::ExecuteLoadProfile_Internal(const FString& PlayerId, FBaruPlayerProfileData& OutProfile)
{
    FScopeLock ScopeLock(&DbLock);

    FSQLiteDatabase Db;
    if (!Db.Open(*DbPath, ESQLiteDatabaseOpenMode::ReadWrite))
    {
        BARU_LOG(LogBaruBackend, Error, TEXT("DB Open Failed for LoadProfile: %s"), *DbPath);
        return false;
    }

    FSQLitePreparedStatement Statement;
    const FString Query = FString::Printf(TEXT("SELECT PlayerId, Nickname, Gold, Survivals, Deaths FROM Users WHERE PlayerId = '%s';"), *PlayerId);

    bool bFound = false;
    if (Statement.Create(Db, *Query, ESQLitePreparedStatementFlags::None))
    {
        if (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
        {
            Statement.GetColumnValueByName(TEXT("PlayerId"), OutProfile.PlayerId);
            Statement.GetColumnValueByName(TEXT("Nickname"), OutProfile.Nickname);
            Statement.GetColumnValueByName(TEXT("Gold"), OutProfile.Gold);
            Statement.GetColumnValueByName(TEXT("Survivals"), OutProfile.Survivals);
            Statement.GetColumnValueByName(TEXT("Deaths"), OutProfile.Deaths);
            bFound = true;
        }
        Statement.Destroy();
    }

    // 신규 플레이어인 경우 기본 데이터 생성
    if (!bFound)
    {
        OutProfile.PlayerId = PlayerId;
        OutProfile.Nickname = TEXT("Operative");
        OutProfile.Gold = 1000;
        OutProfile.Survivals = 0;
        OutProfile.Deaths = 0;

        const FString InsertNewUser = FString::Printf(
            TEXT("INSERT INTO Users (PlayerId, Nickname, Gold, Survivals, Deaths) VALUES ('%s', '%s', %d, %d, %d);"),
            *OutProfile.PlayerId, *OutProfile.Nickname, OutProfile.Gold, OutProfile.Survivals, OutProfile.Deaths
        );
        Db.Execute(*InsertNewUser);
        bFound = true;
        BARU_LOG(LogBaruBackend, Log, TEXT("Created new user profile in DB for Player [%s]."), *PlayerId);
    }

    Db.Close();
    return bFound;
}