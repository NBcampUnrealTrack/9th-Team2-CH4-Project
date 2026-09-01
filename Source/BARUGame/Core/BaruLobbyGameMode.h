#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaruLobbyGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruAllPlayersReadyStatusChanged, bool, bAllReady);

class ABaruGameState;
class ABaruPlayerController;
class ABaruPlayerState;
class ABaruCharacter;

/**
 * MainLobbyLevel 및 각 회사 로비 레벨 전용 게임모드
 * - 세션 접속/퇴장 라이프사이클 관리
 * - 플레이어 레디 상태 동기화 및 전원 준비 완료 검사
 * - 탐사 목표 맵 지정 및 Seamless ServerTravel 실행
 */
UCLASS()
class BARUGAME_API ABaruLobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABaruLobbyGameMode();

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    // 플레이어의 레디 상태가 변경되거나 세션 인원이 변동될 때 호출되어 전원 준비 완료 여부를 재계산
    UFUNCTION(BlueprintCallable, Category = "BARU|Lobby")
    void OnPlayerReadyStatusChanged();

    // 방장이 선택한 목표 탐사 맵 URL 설정
    UFUNCTION(BlueprintCallable, Category = "BARU|Lobby")
    void SetTargetRaidMap(const FString& InMapName);

    // 현재 설정된 목표 탐사 맵 반환
    UFUNCTION(BlueprintPure, Category = "BARU|Lobby")
    FString GetTargetRaidMap() const { return SelectedTargetMapURL; }

    // 방장이 탐사 시작 버튼을 눌렀을 때 심리스 트래블을 통해 B1 던전 또는 회사 로비로 이동
    UFUNCTION(BlueprintCallable, Category = "BARU|Lobby")
    void StartGameRaid(const FString& OverrideTargetMapURL = TEXT(""));

public:
    // 전원 준비 완료 여부 변경 시 UI 활성화/비활성화를 위해 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "BARU|Lobby|Events")
    FOnBaruAllPlayersReadyStatusChanged OnAllPlayersReadyStatusChanged;

protected:
    UPROPERTY(Transient)
    TObjectPtr<ABaruGameState> CachedBaruGameState;

    // 기본 탐사 맵 경로
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Lobby")
    FString DefaultRaidMapURL = TEXT("/Game/BARUGame/Maps/TestGym");

    // 현재 선택된 목적지 맵 URL
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "BARU|Lobby")
    FString SelectedTargetMapURL;
};