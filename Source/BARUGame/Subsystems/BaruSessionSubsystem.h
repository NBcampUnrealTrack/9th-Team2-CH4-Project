#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "UObject/SoftObjectPtr.h"
#include "BaruSessionSubsystem.generated.h"

class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDestroySessionComplete, bool, bWasSuccessful);

UCLASS()
class BARUGAME_API UBaruSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBaruSessionSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void CreateSession(int32 NumPublicConnections = 5, bool bIsLANMatch = false, TSoftObjectPtr<UWorld> OverrideLobbyLevel = nullptr);
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void FindSessions(int32 MaxSearchResults = 20, bool bIsLANMatch = false);
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void JoinSessionByIndex(int32 SessionIndex);
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void DestroySession();
    
    UFUNCTION(BlueprintPure, Category = "BARU|Session")
    int32 GetSearchResultsCount() const;

public:
    // UI 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruCreateSessionComplete OnCreateSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruFindSessionsComplete OnFindSessionsCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruJoinSessionComplete OnJoinSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruDestroySessionComplete OnDestroySessionCompleteEvent;
    
protected:
    /** 기본 트럭 UI 및 방 만들기용 로비 레벨 에셋 레퍼런스 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BARU|Session|Maps")
    TSoftObjectPtr<UWorld> DefaultMainLobbyLevel;

private:
    void OpenLobbyLevelAsListenServer(const TSoftObjectPtr<UWorld>& LevelToOpen);

    FDelegateHandle CreateSessionCompleteDelegateHandle;
};