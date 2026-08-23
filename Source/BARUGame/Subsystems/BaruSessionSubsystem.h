#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BaruSessionSubsystem.generated.h"

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
    void CreateSession(int32 NumPublicConnections = 4, bool bIsLANMatch = false);
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void FindSessions(int32 MaxSearchResults = 20, bool bIsLANMatch = false);
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void JoinSessionByIndex(int32 SessionIndex);
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void DestroySession();
    
    UFUNCTION(BlueprintPure, Category = "BARU|Session")
    int32 GetSearchResultsCount() const;

public:
    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruCreateSessionComplete OnCreateSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruFindSessionsComplete OnFindSessionsCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruJoinSessionComplete OnJoinSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruDestroySessionComplete OnDestroySessionCompleteEvent;
};