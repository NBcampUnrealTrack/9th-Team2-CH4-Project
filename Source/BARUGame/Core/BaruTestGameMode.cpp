#include "Core/BaruTestGameMode.h"
#include "Core/BaruGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Character/BaruCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "BaruLog.h"

ABaruTestGameMode::ABaruTestGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    // 테스트에 필요한 핵심 코어 클래스 연결
    GameStateClass = ABaruGameState::StaticClass();
    PlayerControllerClass = ABaruPlayerController::StaticClass();
    PlayerStateClass = ABaruPlayerState::StaticClass();
    DefaultPawnClass = ABaruCharacter::StaticClass();
}

void ABaruTestGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    BARU_LOG(LogBaruSession, Log, TEXT("[TEST_MODE] ABaruTestGameMode Initialized on Map: %s"), *MapName);
}

void ABaruTestGameMode::BeginPlay()
{
    Super::BeginPlay();

    CachedBaruGameState = GetGameState<ABaruGameState>();
    if (CachedBaruGameState)
    {
        // UI나 로직이 대기 상태에 갇히지 않도록 InProgress로 즉시 전이
        CachedBaruGameState->SetMatchState(EBaruMatchState::InProgress);
    }
}

APawn* ABaruTestGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Instigator = GetInstigator();
    SpawnInfo.ObjectFlags |= RF_Transient;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);

    if (!SpawnedPawn)
    {
        BARU_LOG(LogBaruSession, Error, TEXT("Failed to spawn DefaultPawn for %s at %s"), *GetNameSafe(NewPlayer), *SpawnTransform.ToString());
    }

    return SpawnedPawn;
}

void ABaruTestGameMode::PostLogin(APlayerController* NewPlayer)
{
    // Super::PostLogin에서 PlayerStart 검색 -> Pawn 스폰 -> Possess 자동 수행
    Super::PostLogin(NewPlayer);

    if (IsValid(NewPlayer))
    {
        BARU_NET_LOG(NewPlayer, LogBaruSession, Log, TEXT("[TEST_MODE] Client Connected & Spawned: %s"), *NewPlayer->GetName());
    }

    UpdateTestPlayerCount();
}

void ABaruTestGameMode::Logout(AController* Exiting)
{
    if (IsValid(Exiting))
    {
        BARU_NET_LOG(Exiting, LogBaruSession, Log, TEXT("[TEST_MODE] Client Disconnected: %s"), *Exiting->GetName());

        if (APawn* ControlledPawn = Exiting->GetPawn())
        {
            ControlledPawn->Destroy();
        }
    }

    Super::Logout(Exiting);
    UpdateTestPlayerCount();
}

void ABaruTestGameMode::OnPlayerDied(AController* VictimController, AActor* KillerActor)
{
    if (!IsValid(VictimController)) return;

    BARU_NET_LOG(VictimController, LogBaruCombat, Log, TEXT("[TEST_MODE] Player Died: %s (Killer: %s) -> Scheduling Respawn"),
        *VictimController->GetName(), KillerActor ? *KillerActor->GetName() : TEXT("None"));

    // 기존 폰 정리
    if (APawn* DeadPawn = VictimController->GetPawn())
    {
        DeadPawn->Destroy();
    }

    // 지정된 딜레이 후 자동 리스폰
    if (AutoRespawnDelay > 0.0f)
    {
        FTimerHandle RespawnTimerHandle;
        FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &ABaruTestGameMode::RespawnPlayer, VictimController);
        GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, AutoRespawnDelay, false);
    }
    else
    {
        RespawnPlayer(VictimController);
    }
}

void ABaruTestGameMode::RespawnPlayer(AController* TargetController)
{
    if (!IsValid(TargetController) || TargetController->IsPendingKillPending()) return;

    // 엔진 기본 리스폰 파이프라인 (새 Pawn 스폰 및 빙의)
    RestartPlayer(TargetController);

    // PlayerState의 DBNO 및 상태 초기화
    if (ABaruPlayerState* PS = TargetController->GetPlayerState<ABaruPlayerState>())
    {
        PS->SetDBNOState(false);
        PS->SetSanityValue(100.0f);
    }

    BARU_NET_LOG(TargetController, LogBaruSession, Log, TEXT("[TEST_MODE] Player Respawned: %s"), *TargetController->GetName());
    UpdateTestPlayerCount();
}

void ABaruTestGameMode::UpdateTestPlayerCount()
{
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetAlivePlayerCount(GetNumPlayers());
    }
}