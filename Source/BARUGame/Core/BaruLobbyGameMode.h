#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaruLobbyGameMode.generated.h"

class ABaruLobbyGameState;
class ABaruPlayerController;
class ABaruPlayerState;
class ABaruCharacter;

/**
 * MainLobbyLevel 및 각 회사 로비 레벨 전용 게임모드 (Server Only)
 * - 세션 접속/퇴장 라이프사이클 관리
 * - 플레이어 레디 상태 집계 후 ABaruLobbyGameState로 복제 전파 위임
 * - 탐사 목표 맵 지정 및 Seamless ServerTravel 실행
 */

UCLASS()
class BARUGAME_API ABaruLobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABaruLobbyGameMode();

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    
    virtual void HandleSeamlessTravelPlayer(AController*& C) override;
    virtual void PostSeamlessTravel() override;

    // 플레이어의 레디 상태가 변경되거나 세션 인원이 변동될 때 호출되어 전원 준비 완료 여부를 재계산
    UFUNCTION(BlueprintCallable, Category = "BARU|Lobby")
    void OnPlayerReadyStatusChanged();

    // 방장이 선택한 목표 탐사 맵 URL 설정
    UFUNCTION(BlueprintCallable, Category = "BARU|Lobby")
    void SetTargetRaidMap(const FString& InMapName);
    
    // 서버 전용 헬퍼
    UFUNCTION(BlueprintPure, Category = "BARU|Lobby")
    FString GetTargetRaidMap() const;

    // 방장이 탐사 시작 버튼을 눌렀을 때 심리스 트래블을 통해 B1 던전 또는 회사 로비로 이동
    UFUNCTION(BlueprintCallable, Category = "BARU|Lobby")
    void StartGameRaid(const FString& OverrideTargetMapURL = TEXT(""));

protected:
    UFUNCTION()
    void HandlePlayerReadyStatusChanged(bool bIsReady);
    
    void InitializeLobbyPlayerState(APlayerController* PC);
    
protected:
    UPROPERTY(Transient)
    TObjectPtr<ABaruLobbyGameState> CachedLobbyGameState;

    // 기본 탐사 맵 경로
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Lobby")
    FString DefaultRaidMapURL = TEXT("/Game/BARUGame/Maps/TestGym");
};