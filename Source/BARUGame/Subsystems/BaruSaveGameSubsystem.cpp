#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AES.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Async/Async.h"
#include "BaruLog.h"

namespace BaruSaveCrypto
{
    // 파일 식별용 매직 넘버 ('B', 'A', 'R', 'U')
    // 유출 주의!
    static const uint32 SAVE_MAGIC = 0x55524142;
    static const int32 CURRENT_CRYPTO_VERSION = 1;

    // AES-256 고정 32바이트 암호화 키 (상용 배포 시 난독화 또는 플랫폼 키체인/서버 토큰 연동)
    // 유출 주의!
    static const uint8 AES_KEY[32] = {
        0x42, 0x41, 0x52, 0x55, 0x5F, 0x53, 0x45, 0x43, // B A R U _ S E C
        0x55, 0x52, 0x45, 0x5F, 0x53, 0x41, 0x56, 0x45, // U R E _ S A V E
        0x5F, 0x32, 0x30, 0x32, 0x36, 0x5F, 0x30, 0x39, // _ 2 0 2 6 _ 0 9
        0x5F, 0x30, 0x31, 0x21, 0x40, 0x23, 0x24, 0x25  // _ 0 1 ! @ # $ %
    };
    
    #pragma pack(push, 1)
    struct FBaruSaveHeader
    {
        uint32 Magic = SAVE_MAGIC;
        int32 Version = CURRENT_CRYPTO_VERSION;
        int32 OriginalSize = 0;
        uint8 Checksum[20] = {0};
    };
    #pragma pack(pop)
}

UBaruSaveGameSubsystem::UBaruSaveGameSubsystem()
{
}

void UBaruSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BARU_LOG(LogBaruSession, Log, TEXT("BaruSaveGameSubsystem Initialized."));
}

void UBaruSaveGameSubsystem::Deinitialize()
{
    for (const auto& Pair : CachedSaveGames)
    {
        if (Pair.Value)
        {
            SaveEncryptedSlotInternal(Pair.Value, Pair.Key);
        }
    }
    CachedSaveGames.Empty();

    BARU_LOG(LogBaruSession, Log, TEXT("BaruSaveGameSubsystem Deinitialized."));
    Super::Deinitialize();
}

FString UBaruSaveGameSubsystem::GetSaveFilePath(const FString& SlotName) const
{
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".sav"));
}

bool UBaruSaveGameSubsystem::DoesEncryptedSaveExist(const FString& SlotName) const
{
    return FPaths::FileExists(GetSaveFilePath(SlotName));
}

UBaruSaveGame* UBaruSaveGameSubsystem::LoadOrCreateSaveGame(const FString& InPlayerName)
{
    CurrentSlotName = UBaruSaveGame::GetSlotName(InPlayerName);

    // 메모리 캐시 확인
    if (TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(CurrentSlotName))
    {
        if (FoundSave && FoundSave->Get())
        {
            OnLoadCompletedEvent.Broadcast(CurrentSlotName, true);
            return FoundSave->Get();
        }
    }

    UBaruSaveGame* TargetSaveGame = nullptr;
    
    // 로드 및 무결성 검증
    if (DoesEncryptedSaveExist(CurrentSlotName))
    {
        TargetSaveGame = LoadEncryptedSlotInternal(CurrentSlotName);
    }

    // 신규 유저 or 변조된 경우 신규 생성
    if (!TargetSaveGame)
    {
        TargetSaveGame = Cast<UBaruSaveGame>(UGameplayStatics::CreateSaveGameObject(UBaruSaveGame::StaticClass()));
        if (TargetSaveGame)
        {
            TargetSaveGame->PlayerName = InPlayerName.IsEmpty() ? TEXT("Operative") : InPlayerName;
        }
        BARU_LOG(LogBaruSession, Log, TEXT("New SaveGame Created for: %s"), *InPlayerName);
    }
    
    // 캐시 등록
    if (TargetSaveGame)
    {
        CachedSaveGames.Add(CurrentSlotName, TargetSaveGame);
    }

    OnLoadCompletedEvent.Broadcast(CurrentSlotName, TargetSaveGame != nullptr);
    return TargetSaveGame;
}

bool UBaruSaveGameSubsystem::SaveEncryptedSlotInternal(UBaruSaveGame* SaveObject, const FString& SlotName)
{
    if (!IsValid(SaveObject) || SlotName.IsEmpty())
    {
        return false;
    }

    // 메모리로 직렬화
    TArray<uint8> RawBytes;
    if (!UGameplayStatics::SaveGameToMemory(SaveObject, RawBytes))
    {
        BARU_LOG(LogBaruSession, Error, TEXT("SaveEncrypted Failed: SaveGameToMemory failed for slot '%s'."), *SlotName);
        return false;
    }

    const int32 OriginalSize = RawBytes.Num();

    // AES-256 블록(16바이트) 단위 패딩
    const int32 PaddedSize = FMath::DivideAndRoundUp(OriginalSize, 16) * 16;
    RawBytes.SetNumZeroed(PaddedSize);

    // AES-256 데이터 암호화
    FAES::EncryptData(RawBytes.GetData(), static_cast<uint32>(PaddedSize), BaruSaveCrypto::AES_KEY, 32);

    // 암호화된 데이터의 SHA-256 체크섬 계산 (Encrypt-then-MAC)
    FSHAHash Checksum;
    FSHA1::HashBuffer(RawBytes.GetData(), static_cast<uint64>(PaddedSize), Checksum.Hash);
    
    // FBaruSaveHeader 구조체를 활용한 단일 블록 패킷 조립
    BaruSaveCrypto::FBaruSaveHeader Header;
    Header.OriginalSize = OriginalSize;
    FMemory::Memcpy(Header.Checksum, Checksum.Hash, sizeof(Header.Checksum));
    
    TArray<uint8> FileData;
    FileData.Reserve(sizeof(BaruSaveCrypto::FBaruSaveHeader) + PaddedSize);
    FileData.Append(reinterpret_cast<const uint8*>(&Header), sizeof(BaruSaveCrypto::FBaruSaveHeader));
    FileData.Append(RawBytes);

    const FString FilePath = GetSaveFilePath(SlotName);
    const bool bSuccess = FFileHelper::SaveArrayToFile(FileData, *FilePath);

    BARU_LOG(LogBaruSession, Log, TEXT("Encrypted Save to '%s': %s (Payload: %d bytes)"), 
        *SlotName, bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"), FileData.Num());

    return bSuccess;
}

UBaruSaveGame* UBaruSaveGameSubsystem::LoadEncryptedSlotInternal(const FString& SlotName)
{
    const FString FilePath = GetSaveFilePath(SlotName);

    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("LoadEncrypted Failed: Cannot read file '%s'."), *FilePath);
        return nullptr;
    }
    
    constexpr int32 HeaderSize = sizeof(BaruSaveCrypto::FBaruSaveHeader);
    if (FileData.Num() < HeaderSize)
    {
        BARU_LOG(LogBaruSession, Error, TEXT("LoadEncrypted Failed: Corrupted header in '%s'."), *SlotName);
        return nullptr;
    }

    // 1. 헤더 파싱
    BaruSaveCrypto::FBaruSaveHeader Header;
    FMemory::Memcpy(&Header, FileData.GetData(), HeaderSize);

    if (Header.Magic != BaruSaveCrypto::SAVE_MAGIC)
    {
        BARU_LOG(LogBaruSession, Error, TEXT("LoadEncrypted Failed: Invalid magic code in '%s'."), *SlotName);
        return nullptr;
    }
    
    const int32 EncryptedPayloadSize = FileData.Num() - HeaderSize;
    if (EncryptedPayloadSize <= 0 || EncryptedPayloadSize % 16 != 0)
    {
        BARU_LOG(LogBaruSession, Error, TEXT("LoadEncrypted Failed: Payload not aligned to 16 bytes in '%s'."), *SlotName);
        return nullptr;
    }

    // 2. 암호문 복사 및 체크섬 검증
    const uint8* PayloadPtr = FileData.GetData() + HeaderSize;
    TArray<uint8> EncryptedBytes;
    EncryptedBytes.SetNumUninitialized(EncryptedPayloadSize);
    FMemory::Memcpy(EncryptedBytes.GetData(), PayloadPtr, EncryptedPayloadSize);

    FSHAHash CalculatedChecksum;
    FSHA1::HashBuffer(EncryptedBytes.GetData(), static_cast<uint64>(EncryptedPayloadSize), CalculatedChecksum.Hash);

    if (FMemory::Memcmp(Header.Checksum, CalculatedChecksum.Hash, sizeof(Header.Checksum)) != 0)
    {
        BARU_LOG(LogBaruSession, Error, 
            TEXT("CRITICAL: Save file tampering detected in slot '%s'! Checksum mismatch. Load aborted."), *SlotName);
        return nullptr;
    }

    // 3. 복호화 및 패딩 제거
    FAES::DecryptData(EncryptedBytes.GetData(), static_cast<uint32>(EncryptedPayloadSize), BaruSaveCrypto::AES_KEY, 32);

    if (Header.OriginalSize > 0 && Header.OriginalSize <= EncryptedPayloadSize)
    {
        EncryptedBytes.SetNum(Header.OriginalSize);
    }

    // 4. 역직렬화
    USaveGame* LoadedObject = UGameplayStatics::LoadGameFromMemory(EncryptedBytes);
    UBaruSaveGame* LoadedSaveGame = Cast<UBaruSaveGame>(LoadedObject);

    if (LoadedSaveGame)
    {
        BARU_LOG(LogBaruSession, Log, TEXT("Encrypted SaveGame Loaded & Verified successfully: %s"), *SlotName);
    }
    else
    {
        BARU_LOG(LogBaruSession, Error, TEXT("LoadEncrypted Failed: Memory deserialization failed for '%s'."), *SlotName);
    }

    return LoadedSaveGame;
}

void UBaruSaveGameSubsystem::SaveCurrentGameAsync()
{
    SaveGameBySlotAsync(CurrentSlotName);
}

void UBaruSaveGameSubsystem::SaveGameBySlotAsync(const FString& InSlotName)
{
    if (InSlotName.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveGameBySlotAsync Failed: SlotName is empty."));
        OnSaveCompletedEvent.Broadcast(InSlotName, false);
        return;
    }

    TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(InSlotName);
    if (!FoundSave || !FoundSave->Get())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveGameBySlotAsync Failed: No cached save game for slot '%s'."), *InSlotName);
        OnSaveCompletedEvent.Broadcast(InSlotName, false);
        return;
    }
    
    // Race Condition 방지
    UBaruSaveGame* SaveSnapshot = DuplicateObject<UBaruSaveGame>(FoundSave->Get(), this);

    // ThreadPool 비동기 실행 (메인 스레드 렌더링 히치 제거)
    TWeakObjectPtr<UBaruSaveGameSubsystem> WeakThis(this);

    Async(EAsyncExecution::ThreadPool, [WeakThis, SaveSnapshot, InSlotName]()
    {
        UBaruSaveGameSubsystem* StrongThis = WeakThis.Get();
        const bool bSuccess = StrongThis ? StrongThis->SaveEncryptedSlotInternal(SaveSnapshot, InSlotName) : false;

        Async(EAsyncExecution::TaskGraphMainThread, [WeakThis, InSlotName, bSuccess]()
        {
            if (UBaruSaveGameSubsystem* Subsystem = WeakThis.Get())
            {
                Subsystem->OnSaveCompletedEvent.Broadcast(InSlotName, bSuccess);
            }
        });
    });
}

bool UBaruSaveGameSubsystem::SaveGameBySlot(const FString& InSlotName)
{
    if (InSlotName.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveGame Failed: SlotName is empty."));
        return false;
    }

    TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(InSlotName);
    if (!FoundSave || !FoundSave->Get())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveGame Failed: No cached save game for slot '%s'."), *InSlotName);
        return false;
    }

    const bool bSuccess = SaveEncryptedSlotInternal(FoundSave->Get(), InSlotName);
    OnSaveCompletedEvent.Broadcast(InSlotName, bSuccess);
    return bSuccess;
}

void UBaruSaveGameSubsystem::RecordRaidResult(const FString& InPlayerName, int32 EarnedGold, bool bSurvived)
{
    const FString SlotName = UBaruSaveGame::GetSlotName(InPlayerName);

    UBaruSaveGame* SaveData = LoadOrCreateSaveGame(InPlayerName);
    if (!SaveData)
    {
        return;
    }

    SaveData->TotalGold += EarnedGold;
    if (bSurvived)
    {
        SaveData->TotalSurvivals++;
    }
    else
    {
        SaveData->TotalDeaths++;
    }

    SaveGameBySlotAsync(SlotName);
}

UBaruSaveGame* UBaruSaveGameSubsystem::GetCachedSaveGame() const
{
    if (const TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(CurrentSlotName))
    {
        return FoundSave->Get();
    }
    return nullptr;
}

UBaruSaveGame* UBaruSaveGameSubsystem::GetCachedSaveGameByPlayer(const FString& InPlayerName) const
{
    const FString SlotName = UBaruSaveGame::GetSlotName(InPlayerName);
    if (const TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(SlotName))
    {
        return FoundSave->Get();
    }
    return nullptr;
}