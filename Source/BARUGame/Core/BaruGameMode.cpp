#include "Core/BaruGameMode.h"
#include "Core/BaruGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Character/BaruCharacter.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"

ABaruGameMode::ABaruGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    // 인게임에서도 다음 탐사 또는 로비 복귀를 위해 Seamless Travel 활성화
    bUseSeamlessTravel = true;

    GameStateClass = ABaruGameState::StaticClass();
    PlayerControllerClass = ABaruPlayerController::StaticClass();
    PlayerStateClass = ABaruPlayerState::StaticClass();
    DefaultPawnClass = ABaruCharacter::StaticClass();
}

void ABaruGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    BARU_LOG(LogBaruSession, Log, TEXT("ABaruGameMode::InitGame on Ingame Map: %s"), *MapName);
}

void ABaruGameMode::BeginPlay()
{
    Super::BeginPlay();

    CachedBaruGameState = GetGameState<ABaruGameState>();

    // 인게임 진입 즉시 탐사 상태로 전환 및 레이드 타이머 시작
    SetMatchPhase(EBaruMatchState::InProgress);
    StartRaidTimer();
}

void ABaruGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!IsValid(NewPlayer))
    {
        return;
    }

    BARU_NET_LOG(NewPlayer, LogBaruSession, Log, TEXT("Ingame Player Logged In: %s"), *NewPlayer->GetName());

    // 재접속 핸드셰이크 검증
    if (ABaruPlayerState* NewPS = NewPlayer->GetPlayerState<ABaruPlayerState>())
    {
        const FString IdStr = NewPS->GetUniqueId().ToString();

        if (FDisconnectedPlayerSnapshot* Found = DisconnectedSnapshots.Find(IdStr))
        {
            const float Elapsed = GetWorld()->GetTimeSeconds() - Found->DisconnectTime;

            // 유예 시간 이내이며 기존 폰이 월드에 살아있는 경우
            if (Elapsed <= ReconnectGracePeriod && Found->PreservedPawn.IsValid())
            {
                // Super::PostLogin에서 엔진이 자동 스폰한 임시 기본 폰 제거
                if (APawn* DefaultSpawnedPawn = NewPlayer->GetPawn())
                {
                    if (DefaultSpawnedPawn != Found->PreservedPawn.Get())
                    {
                        DefaultSpawnedPawn->Destroy();
                    }
                }

                // 월드에 남아있던 원래 폰에 재빙의
                NewPlayer->Possess(Found->PreservedPawn.Get());
                DisconnectedSnapshots.Remove(IdStr);

                BARU_NET_LOG(NewPlayer, LogBaruSession, Log, 
                    TEXT("Player successfully reconnected and re-possessed original Pawn (NetId: %s)"), *IdStr);

                UpdateAlivePlayerCount();
                return;
            }
            else
            {
                // 유예 시간이 지났거나 폰이 파괴된 경우 정리
                if (Found->PreservedPawn.IsValid())
                {
                    Found->PreservedPawn->Destroy();
                }
                DisconnectedSnapshots.Remove(IdStr);
            }
        }
    }

    UpdateAlivePlayerCount();
}

void ABaruGameMode::Logout(AController* Exiting)
{
    if (APlayerController* PC = Cast<APlayerController>(Exiting))
    {
        BARU_NET_LOG(PC, LogBaruSession, Log, TEXT("Ingame Player Logged Out: %s"), *PC->GetName());

        APawn* ExitingPawn = PC->GetPawn();
        ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>();

        // 1. 생존 상태의 플레이어가 튕긴 경우: 폰을 파괴하지 않고 보존
        if (ExitingPawn && PS && PS->IsAlive())
        {
            FDisconnectedPlayerSnapshot Snapshot;
            Snapshot.UniqueId = PS->GetUniqueId();
            Snapshot.PreservedPawn = ExitingPawn;
            Snapshot.DisconnectTime = GetWorld()->GetTimeSeconds();

            FString IdStr = Snapshot.UniqueId.IsValid() ? Snapshot.UniqueId.ToString() : FString();
            if (IdStr.IsEmpty())
            {
                IdStr = PS->GetPlayerName().IsEmpty() ? PC->GetName() : PS->GetPlayerName();
            }

            DisconnectedSnapshots.Add(IdStr, Snapshot);

            BARU_NET_LOG(PC, LogBaruSession, Warning, 
                TEXT("Player disconnected while ALIVE. Pawn preserved for %.0fs (NetId: %s)"), 
                ReconnectGracePeriod, *IdStr);

            // 다른 관전자가 이 폰을 보고 있었다면 다른 생존자로 시점 변경
            for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
            {
                APlayerController* OtherPC = Iterator->Get();
                if (IsValid(OtherPC) && OtherPC != PC && OtherPC->GetViewTarget() == ExitingPawn)
                {
                    StartSpectating(OtherPC);
                }
            }

            // 폰을 월드에 그대로 남겨두기 위해 빙의만 해제
            PC->UnPossess();

            Super::Logout(Exiting);
            UpdateAlivePlayerCount();
            return;
        }

        // 2. 이미 사망했거나 폰이 없는 경우 정상 파괴
        if (ExitingPawn)
        {
            ExitingPawn->Destroy();
        }
    }

    Super::Logout(Exiting);
    UpdateAlivePlayerCount();
}

void ABaruGameMode::SetMatchPhase(EBaruMatchState NewPhase)
{
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetMatchState(NewPhase);
    }
}

void ABaruGameMode::StartRaidTimer()
{
    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetRemainingRaidTime(RaidDurationInSeconds);
    }

    GetWorldTimerManager().SetTimer(
        RaidCountdownTimerHandle,
        this,
        &ABaruGameMode::UpdateRaidCountdown,
        1.0f,
        true
    );
    BARU_LOG(LogBaruSession, Log, TEXT("Raid Countdown Timer Started: %d Seconds"), RaidDurationInSeconds);
}

void ABaruGameMode::UpdateRaidCountdown()
{
    if (!CachedBaruGameState) return;

    CleanUpExpiredSnapshots();

    const int32 CurrentTime = CachedBaruGameState->GetRemainingRaidTime();
    if (CurrentTime > 0)
    {
        CachedBaruGameState->SetRemainingRaidTime(CurrentTime - 1);
        
        if (CurrentTime == 60)
        {
            CachedBaruGameState->Multicast_BroadcastNotification(
                FText::FromString(TEXT("WARNING: Facility Shutdown in 60 Seconds")), 5.0f);
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(RaidCountdownTimerHandle);
        OnRaidTimeout();
    }
}

void ABaruGameMode::OnRaidTimeout()
{
    BARU_LOG(LogBaruSession, Warning, TEXT("Raid Time Expired! Facility Locked Down."));

    if (CachedBaruGameState)
    {
        CachedBaruGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("TIME OUT: Facility Locked. Extraction Failed.")), 5.0f);
    }

    // 시간 초과 시 전원 탈출 실패 정산 실행
    ProcessSettlement(false);
}

void ABaruGameMode::OnExtractionZoneCountChanged(int32 InZoneCount)
{
    if (!CachedBaruGameState) return;

    CachedBaruGameState->SetPlayersInExtractionZoneCount(InZoneCount);

    const int32 AliveCount = CachedBaruGameState->GetAlivePlayerCount();
    if (AliveCount > 0 && InZoneCount >= AliveCount)
    {
        BARU_LOG(LogBaruSession, Log, TEXT("All Alive Players In Extraction Zone. Ready for Departure."));
        SetMatchPhase(EBaruMatchState::Extraction);
    }
}

void ABaruGameMode::RequestLevelTransition(const FString& TargetMapURL)
{
    // 목적지가 비어있을 경우 DefaultReturnMapURL로 FallBack
    FString FinalTargetMapURL = TargetMapURL;
    if (FinalTargetMapURL.IsEmpty())
    {
        FinalTargetMapURL = DefaultReturnMapURL;
        BARU_LOG(LogBaruSession, Warning, TEXT("TargetMapURL was empty. Fallback to DefaultReturnMapURL: %s"), *DefaultReturnMapURL);
    }

    if (GetWorldTimerManager().IsTimerActive(LevelTransitionTimerHandle)) return;

    PendingTargetMapURL = FinalTargetMapURL;
    SetMatchPhase(EBaruMatchState::Extraction);
    
    ProcessSettlement(true);

    // 모든 클라이언트에 안도 시네마틱 연출 브로드캐스트
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(Iterator->Get()))
        {
            if (IsValid(BaruPC) && !BaruPC->IsPendingKillPending())
            {
                BaruPC->Client_PlayElevatorCinematic();
            }
        }
    }

    // 연출 시간 대기 후 실제 ServerTravel 실행 : 8초 대기
    const float SafeTransitionDelay = FMath::Max(TransitionDelayDuration, 8.0f);

    BARU_NET_LOG(this, LogBaruSession, Log, 
        TEXT("Level transition requested. Traveling to '%s' in %.1f seconds..."), 
        *PendingTargetMapURL, SafeTransitionDelay);

    GetWorldTimerManager().SetTimer(
        LevelTransitionTimerHandle,
        this,
        &ABaruGameMode::ExecuteServerTravel,
        SafeTransitionDelay,
        false
    );
}

void ABaruGameMode::ExecuteServerTravel()
{
    if (PendingTargetMapURL.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Error, TEXT("ExecuteServerTravel Failed: Empty Target Map URL."));
        return;
    }
    
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Executing ServerTravel to: %s"), *PendingTargetMapURL);
    GetWorld()->ServerTravel(PendingTargetMapURL + TEXT("?listen"), true);
}

void ABaruGameMode::UpdateAlivePlayerCount()
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown)
    {
        return;
    }
    
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (!CachedBaruGameState) return;

    int32 CurrentActive = 0;
    int32 CurrentDBNO = 0;
    
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (IsValid(PC) && !PC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsAlive()) 
                {
                    CurrentActive++;
                }

                else if (PS->IsDBNO() && !PS->IsDead()) 
                {
                    CurrentDBNO++;
                }
            }
        }
    }

    CurrentActive += DisconnectedSnapshots.Num();

    CachedBaruGameState->SetAlivePlayerCount(CurrentActive);

    // 완전 전멸(접속자 0 + 대기자 0) 시 전멸 검사
    if (CachedBaruGameState->GetMatchState() == EBaruMatchState::InProgress)
    {
        if (CurrentActive <= 0 && CurrentDBNO <= 0)
        {
            CheckTeamWipe();
        }
    }
}

void ABaruGameMode::OnPlayerDied(AController* VictimController, AActor* KillerActor)
{
    BARU_NET_LOG(VictimController, LogBaruCombat, Log, TEXT("Player Died: %s (Killer: %s)"),
        VictimController ? *VictimController->GetName() : TEXT("None"),
        KillerActor ? *KillerActor->GetName() : TEXT("None"));

    UpdateAlivePlayerCount();

    if (APlayerController* VictimPC = Cast<APlayerController>(VictimController))
    {
        if (APawn* DeadPawn = VictimPC->GetPawn())
        {
            VictimPC->SetViewTargetWithBlend(DeadPawn, 0.5f);
            if (!VictimPC->IsLocalController())
            {
                VictimPC->ClientSetViewTarget(DeadPawn, FViewTargetTransitionParams());
            }
        }
        VictimPC->UnPossess();
        
        if (CachedBaruGameState && CachedBaruGameState->GetAlivePlayerCount() > 0)
        {
            FTimerHandle SpectateTimerHandle;
            FTimerDelegate SpectateDelegate = FTimerDelegate::CreateUObject(this, &ABaruGameMode::StartSpectating, VictimPC);
            GetWorldTimerManager().SetTimer(SpectateTimerHandle, SpectateDelegate, DeathSpectateDelay, false);
        }
    }
}

void ABaruGameMode::StartSpectating(APlayerController* DeadController)
{
    if (!IsValid(DeadController) || DeadController->IsPendingKillPending()) return;

    DeadController->StartSpectatingOnly();
    
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* OtherPC = Iterator->Get();
        if (IsValid(OtherPC) && OtherPC != DeadController && !OtherPC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = OtherPC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsAlive() && OtherPC->GetPawn())
                {
                    DeadController->SetViewTargetWithBlend(OtherPC->GetPawn(), 1.0f);

                    if (!DeadController->IsLocalController())
                    {
                        DeadController->ClientSetViewTarget(OtherPC->GetPawn(), FViewTargetTransitionParams());
                    }
                    BARU_NET_LOG(DeadController, LogBaruSession, Log, TEXT("Spectating Target Set to: %s"), *OtherPC->GetName());
                    return;
                }
            }
        }
    }
}

void ABaruGameMode::CheckTeamWipe()
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown || !CachedBaruGameState)
    {
        return;
    }
    
    if (CachedBaruGameState->GetMatchState() != EBaruMatchState::InProgress)
    {
        return;
    }
    
    // DBNO 상태인 플레이어가 1명이라도 있으면 전멸이 아님 (방어 코드)
    for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (APlayerController* PC = Iterator->Get())
        {
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsDBNO() && !PS->IsDead())
                {
                    return;
                }
            }
        }
    }
    
    if (CachedBaruGameState->GetAlivePlayerCount() <= 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("Team wiped. Processing Failure Settlement."));

        // 플레이어 전멸 시 패배 신호 발송
        CachedBaruGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("MISSION FAILED: All Operatives Lost")), 5.0f);
        
        GetWorldTimerManager().ClearTimer(LevelTransitionTimerHandle);
        GetWorldTimerManager().ClearTimer(RaidCountdownTimerHandle);

        ProcessSettlement(false);
    }
}

void ABaruGameMode::AddTeamScrapValue(int32 ScrapValue)
{
    if (!CachedBaruGameState || ScrapValue <= 0) return;

    const int32 NewValue = CachedBaruGameState->GetTeamScrapValue() + ScrapValue;
    CachedBaruGameState->SetTeamScrapValue(NewValue);
}

void ABaruGameMode::ProcessSettlement(bool bAllExtracted)
{
    SetMatchPhase(EBaruMatchState::PostGame);

    // 데디케이티드 서버라면 GameMode에서 세이브를 관리하지만, 로컬 .sav 저장 시스템에서는 GameMode가 저장을 해서는 안됨
    // UBaruSaveGameSubsystem* SaveSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UBaruSaveGameSubsystem>() : nullptr;

    // 팀 스크랩 가치 기반 시작
    int32 FinalTeamTotalValue = CachedBaruGameState ? CachedBaruGameState->GetTeamScrapValue() : 0;

    // 생존 탈출에 성공한 대원들이 들고 온 인벤토리 아이템 가치(SettlementValue) 총합 합산
    if (bAllExtracted)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
           if (APlayerController* PC = It->Get())
           {
                const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>();
                if (PS && PS->IsAlive())
                {
                    if (const UBaruInventoryComponent* Inven = PS->GetInventoryComponent())
                    {
                        FinalTeamTotalValue += Inven->GetTotalSettlementValue();
                    }
                }
            }
        }

        // 합산된 최종 수집품 가치를 GameState에도 동기화 갱신
        if (CachedBaruGameState)
        {
            CachedBaruGameState->SetTeamScrapValue(FinalTeamTotalValue);
        }
    }
    
    BARU_NET_LOG(this, LogBaruSession, Log, 
        TEXT("Processing Settlement (Survived: %d, Final Total Team Value: %d)"), 
        bAllExtracted, FinalTeamTotalValue);
    
    // 3. 각 플레이어별 정산 리포트 생성 및 클라이언트 전송
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(Iterator->Get());
        if (!IsValid(BaruPC) || BaruPC->IsPendingKillPending())
        {
            continue;
        }

        const ABaruPlayerState* PS = BaruPC->GetPlayerState<ABaruPlayerState>();
        
        // 생존 여부 판정
        const bool bPlayerSurvived = bAllExtracted && (PS && PS->IsAlive());
        
        // 생존 시 100% 분배, 사망/낙오 시 10% 위로금 지급
        const int32 EarnedGold = bPlayerSurvived ? FinalTeamTotalValue : FMath::RoundToInt(FinalTeamTotalValue * 0.1f);

        // 파밍 아이템 총 개수 집계
        int32 ExtractedItemCount = 0;
        if (bPlayerSurvived && PS)
        {
            if (const UBaruInventoryComponent* InvenComp = PS->GetInventoryComponent())
            {
                ExtractedItemCount = InvenComp->GetTotalItemCount();
            }
        }

        // 몬스터 처치 수 조회
        const int32 Kills = PS ? PS->GetMonsterKillCount() : 0;

        // 정산 DTO 구조체 생성 및 값 초기화 (선언 후 대입)
        FBaruSettlementReport Report;
        Report.bSurvived = bPlayerSurvived;
        Report.AcquiredCurrency = EarnedGold;
        Report.ExtractedItemCount = ExtractedItemCount;
        Report.MonsterKillCount = Kills;

        // 클라이언트에 정산 UI 브로드캐스트 (내부에서 로컬 .sav 저장 동시 진행)
        BaruPC->Client_ShowSettlementUI(Report);
    }
    
    // 탈출 실패(전멸/타임오버) 시 일정 시간 대기 후 로비로 강제 복귀
    if (!bAllExtracted)
    {
        PendingTargetMapURL = DefaultReturnMapURL;
        GetWorldTimerManager().SetTimer(
            LevelTransitionTimerHandle,
            this,
            &ABaruGameMode::ExecuteServerTravel,
            PostSettlementReturnDelay,
            false
        );
    }
}

void ABaruGameMode::CleanUpExpiredSnapshots()
{
    if (DisconnectedSnapshots.Num() == 0) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    TArray<FString> ExpiredIds;

    for (auto& Pair : DisconnectedSnapshots)
    {
        if (CurrentTime - Pair.Value.DisconnectTime > ReconnectGracePeriod)
        {
            ExpiredIds.Add(Pair.Key);
            if (Pair.Value.PreservedPawn.IsValid())
            {
                BARU_NET_LOG(this, LogBaruSession, Warning, 
                    TEXT("Reconnect grace period expired for NetId: %s. Destroying pawn."), *Pair.Key);
                Pair.Value.PreservedPawn->Destroy();
            }
        }
    }

    for (const FString& ExpiredId : ExpiredIds)
    {
        DisconnectedSnapshots.Remove(ExpiredId);
    }

    if (ExpiredIds.Num() > 0)
    {
        UpdateAlivePlayerCount();
    }
}

void ABaruGameMode::RegisterExpectedSpawns(int32 ExpectedCount)
{
    if (ExpectedCount <= 0) return;
    PendingSpawnMonsterCount += ExpectedCount;
    TotalSpawnedMonsters += ExpectedCount;

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetMonsterCounts(TotalSpawnedMonsters, ActiveMonsters.Num() + PendingSpawnMonsterCount);
    }
}

void ABaruGameMode::RegisterMonster(ABaruMonsterCharacter* Monster)
{
    if (!IsValid(Monster) || ActiveMonsters.Contains(Monster)) return;

    ActiveMonsters.Add(Monster);

    // 스포너 대기 카운트에서 1 차감
    if (PendingSpawnMonsterCount > 0)
    {
        PendingSpawnMonsterCount--;
    }
    else
    {
        TotalSpawnedMonsters++;
    }

    if (!CachedBaruGameState) CachedBaruGameState = GetGameState<ABaruGameState>();
    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetMonsterCounts(TotalSpawnedMonsters, ActiveMonsters.Num() + PendingSpawnMonsterCount);
    }
}

void ABaruGameMode::UnregisterMonster(ABaruMonsterCharacter* Monster)
{
    // Die를 거치지 않고 엔진에 의해 강제 파괴된 경우 (예: 낙하/언로드)
    if (ActiveMonsters.Contains(Monster))
    {
        ActiveMonsters.Remove(Monster);
        if (CachedBaruGameState)
        {
            CachedBaruGameState->SetMonsterCounts(TotalSpawnedMonsters, ActiveMonsters.Num() + PendingSpawnMonsterCount);
        }
    }
}

void ABaruGameMode::OnMonsterDied(ABaruMonsterCharacter* Monster, AActor* Killer)
{
    if (!IsValid(Monster)) return;
    
    ActiveMonsters.Remove(Monster);

    // Killer가 유효할 경우 PlayerState 역추적 후 개인 킬 카운트 누적
    if (IsValid(Killer))
    {
        ABaruPlayerState* KillerPS = nullptr;
        if (APawn* KillerPawn = Cast<APawn>(Killer))
        {
            KillerPS = KillerPawn->GetPlayerState<ABaruPlayerState>();
        }
        else if (AController* KillerPC = Cast<AController>(Killer))
        {
            KillerPS = KillerPC->GetPlayerState<ABaruPlayerState>();
        }
        // Killer 자체가 PlayerState로 직접 전달된 경우 대응
        else if (ABaruPlayerState* DirectPS = Cast<ABaruPlayerState>(Killer))
        {
            KillerPS = DirectPS;
        }

        if (KillerPS)
        {
            KillerPS->AddMonsterKill();
        }
    }

    // GameState 팀 총합 킬 수 및 잔여 몬스터 수 갱신
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetTeamMonsterKillCount(CachedBaruGameState->GetTeamMonsterKillCount() + 1);
        CachedBaruGameState->SetMonsterCounts(TotalSpawnedMonsters, ActiveMonsters.Num() + PendingSpawnMonsterCount);

        // 스포너 소환 대기(Pending)와 필드 잔여(Active)가 모두 0일 때만 적 전멸 판정
        if (ActiveMonsters.Num() <= 0 && PendingSpawnMonsterCount <= 0 && TotalSpawnedMonsters > 0)
        {
            CachedBaruGameState->Multicast_NotifyAllMonstersEliminated();
            CachedBaruGameState->Multicast_BroadcastNotification(FText::FromString(TEXT("ALL HOSTILES ELIMINATED")), 5.0f);
        }
    }
}
