#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Items/Spawning/BaruMonsterLootTypes.h"
#include "BaruMonsterItemSpawnerComponent.generated.h"

class ABaruCorpseLootActor;
class APawn;
class UBaruInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaruCorpseLootUpdated);

// 기존 클래스 이름을 유지하되, 바닥 생성 대신 시체의 전리품 목록을 관리합니다.
UCLASS(ClassGroup = (BARU), meta = (BlueprintSpawnableComponent))
class BARUGAME_API UBaruMonsterItemSpawnerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBaruMonsterItemSpawnerComponent();
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 몬스터 사망 확정 후 서버에서 한 번 호출. F 입력에서는 호출하지 않습니다.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Corpse Loot")
    void GenerateLootOnServer();

    // 기존 사망 이벤트의 호출을 깨뜨리지 않기 위한 호환 함수입니다.
    // 이제 바닥에 아이템을 생성하지 않고 GenerateLootOnServer로 전달합니다.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Corpse Loot",
        meta = (DeprecatedFunction, DeprecationMessage = "Use GenerateLootOnServer."))
    void SpawnDropsOnce();

    // 플레이어의 기존 서버 상호작용 경로에서 호출. 반환값은 실제 습득 수량입니다.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Corpse Loot")
    int32 LootAllOnServer(APawn* Interactor);

    UFUNCTION(BlueprintPure, Category = "BARU|Corpse Loot")
    bool HasLoot() const;

    UFUNCTION(BlueprintPure, Category = "BARU|Corpse Loot")
    TArray<FBaruCorpseLootEntry> GetLootEntries() const { return LootEntries; }

    bool CanLoot(APawn* Interactor) const;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Corpse Loot")
    FOnBaruCorpseLootUpdated OnLootUpdated;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 플레이어 Inventory와 같은 DT_Item 에셋.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Corpse Loot")
    TObjectPtr<UDataTable> ItemDataTable;

    // 행 구조가 BaruMonsterLootRow인 몬스터별 테이블.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Corpse Loot")
    TObjectPtr<UDataTable> LootTable;

    // 몬스터는 기본적으로 잡템만 보유. 무기 전리품을 허용할 때만 체크합니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Corpse Loot")
    bool bAllowWeaponDrops = false;

    // 시체의 몸통 본/소켓을 지정하면 그 위치에 상호작용 영역이 붙습니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Corpse Loot|Interaction")
    FName LootAttachBone = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Corpse Loot|Interaction")
    FVector LootRelativeOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Corpse Loot|Interaction",
        meta = (ClampMin = "20.0", ClampMax = "200.0"))
    float InteractionRadius = 80.0f;

private:
    UPROPERTY(ReplicatedUsing = OnRep_LootState)
    bool bLootGenerated = false;

    UPROPERTY(ReplicatedUsing = OnRep_LootState)
    TArray<FBaruCorpseLootEntry> LootEntries;

    // 서버에서만 보관. 반대 방향의 SourceLootComponent 참조는 Actor가 복제합니다.
    UPROPERTY(Transient)
    TObjectPtr<ABaruCorpseLootActor> LootTargetActor;

    bool bGenerationAttempted = false;
    bool bTransferInProgress = false;

    UFUNCTION()
    void OnRep_LootState();

    UBaruInventoryComponent* FindPlayerInventory(APawn* Interactor) const;
    bool IsDeadOwner() const;
    void CreateLootTargetOnServer();
    void NotifyLootUpdatedOnServer();
};
