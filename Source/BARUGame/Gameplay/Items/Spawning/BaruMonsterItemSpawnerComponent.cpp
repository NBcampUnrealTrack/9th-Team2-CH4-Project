#include "Gameplay/Items/Spawning/BaruMonsterItemSpawnerComponent.h"

#include "BaruLog.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"
#include "Gameplay/Items/Spawning/BaruCorpseLootActor.h"
#include "Interfaces/CombatInterface.h"
#include "Net/UnrealNetwork.h"
#include "Templates/UnrealTemplate.h"

UBaruMonsterItemSpawnerComponent::UBaruMonsterItemSpawnerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UBaruMonsterItemSpawnerComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UBaruMonsterItemSpawnerComponent, bLootGenerated);
    DOREPLIFETIME(UBaruMonsterItemSpawnerComponent, LootEntries);
}

bool UBaruMonsterItemSpawnerComponent::IsDeadOwner() const
{
    AActor* Monster = GetOwner();
    return IsValid(Monster)
        && Monster->GetClass()->ImplementsInterface(UCombatInterface::StaticClass())
        && ICombatInterface::Execute_IsDead(Monster);
}

void UBaruMonsterItemSpawnerComponent::SpawnDropsOnce()
{
    // 기존 사망 이벤트의 호출과 호환되도록 새 구현에 전달합니다.
    GenerateLootOnServer();
}

void UBaruMonsterItemSpawnerComponent::GenerateLootOnServer()
{
    AActor* Monster = GetOwner();
    if (!IsValid(Monster) || !Monster->HasAuthority() || bGenerationAttempted)
    {
        return;
    }
    if (!IsDeadOwner())
    {
        BARU_NET_LOG(Monster, LogBaruItem, Warning,
            TEXT("[CorpseLoot] 생성 거절: 사망 확정 후 호출해야 합니다."));
        return;
    }

    // 설정 오류도 동일 시체에서 반복 추첨하지 않습니다. 설정 수정 후 PIE를 재시작합니다.
    bGenerationAttempted = true;
    if (!IsValid(ItemDataTable) || ItemDataTable->GetRowStruct() != FItemData::StaticStruct()
        || !IsValid(LootTable) || LootTable->GetRowStruct() != FBaruMonsterLootRow::StaticStruct())
    {
        BARU_NET_LOG(Monster, LogBaruItem, Error,
            TEXT("[CorpseLoot] DT_Item 또는 BaruMonsterLootRow 구조의 LootTable 설정 오류."));
        return;
    }

    const TArray<FName> RowNames = LootTable->GetRowNames();
    if (RowNames.Num() > 128)
    {
        BARU_NET_LOG(Monster, LogBaruItem, Error,
            TEXT("[CorpseLoot] LootTable은 몬스터당 최대 128행입니다."));
        return;
    }

    // 추첨 전에 모든 행을 검증하여, 잘못된 설정으로 일부만 생성되는 일을 막습니다.
    TSet<FName> SeenItems;
    TArray<FBaruMonsterLootRow> Candidates;
    for (const FName RowName : RowNames)
    {
        const FBaruMonsterLootRow* Row = LootTable->FindRow<FBaruMonsterLootRow>(
            RowName, TEXT("CorpseLoot"));
        if (!Row || Row->ItemRow.DataTable != ItemDataTable.Get()
            || Row->ItemRow.RowName.IsNone()
            || !FMath::IsFinite(Row->DropChance)
            || Row->DropChance < 0.0f || Row->DropChance > 1.0f
            || Row->MinQuantity < 1 || Row->MaxQuantity < Row->MinQuantity
            || Row->MaxQuantity > 10000 || SeenItems.Contains(Row->ItemRow.RowName))
        {
            BARU_NET_LOG(Monster, LogBaruItem, Error,
                TEXT("[CorpseLoot] 행 설정 오류 또는 중복 ItemID: %s"), *RowName.ToString());
            return;
        }
        SeenItems.Add(Row->ItemRow.RowName);
        const FItemData* Data = ItemDataTable->FindRow<FItemData>(
            Row->ItemRow.RowName, TEXT("CorpseLoot"));
        if (!Data || Data->GridSize.X < 1 || Data->GridSize.Y < 1
            || (Data->bStackable && Data->MaxStackSize < 1))
        {
            BARU_NET_LOG(Monster, LogBaruItem, Error,
                TEXT("[CorpseLoot] DT_Item 행/크기/중첩 설정 오류: %s"),
                *Row->ItemRow.RowName.ToString());
            return;
        }
        if (Row->DropChance <= 0.0f
            || (!bAllowWeaponDrops && Data->ItemType == EItemType::Weapon))
        {
            continue;
        }
        // 인벤토리에 직접 넣으므로 ItemActorClass(바닥 Pickup BP)는 요구하지 않습니다.
        Candidates.Add(*Row);
    }

    for (const FBaruMonsterLootRow& Row : Candidates)
    {
        if (Row.DropChance < 1.0f && FMath::FRand() >= Row.DropChance)
        {
            continue;
        }
        FBaruCorpseLootEntry& Entry = LootEntries.AddDefaulted_GetRef();
        Entry.EntryID = FGuid::NewGuid();
        Entry.ItemID = Row.ItemRow.RowName;
        Entry.Quantity = FMath::RandRange(Row.MinQuantity, Row.MaxQuantity);
    }

    bLootGenerated = true; // 0개인 결과도 확정입니다.
    if (HasLoot())
    {
        CreateLootTargetOnServer();
    }
    NotifyLootUpdatedOnServer();
    BARU_NET_LOG(Monster, LogBaruItem, Log,
        TEXT("[CorpseLoot] 목록 확정: 항목=%d"), LootEntries.Num());
}

void UBaruMonsterItemSpawnerComponent::CreateLootTargetOnServer()
{
    AActor* Monster = GetOwner();
    if (!IsValid(Monster) || !Monster->HasAuthority() || !GetWorld()
        || IsValid(LootTargetActor))
    {
        return;
    }

    USceneComponent* AttachParent = Monster->GetRootComponent();
    FName AttachBone = NAME_None;
    if (ACharacter* Character = Cast<ACharacter>(Monster))
    {
        if (USkeletalMeshComponent* Mesh = Character->GetMesh())
        {
            AttachParent = Mesh;
            if (!LootAttachBone.IsNone())
            {
                if (Mesh->DoesSocketExist(LootAttachBone)
                    || Mesh->GetBoneIndex(LootAttachBone) != INDEX_NONE)
                {
                    AttachBone = LootAttachBone;
                }
                else
                {
                    BARU_NET_LOG(Monster, LogBaruItem, Warning,
                        TEXT("[CorpseLoot] 부착 본 없음: %s. Mesh 원점 사용."),
                        *LootAttachBone.ToString());
                }
            }
        }
    }
    if (!IsValid(AttachParent))
    {
        BARU_NET_LOG(Monster, LogBaruItem, Error,
            TEXT("[CorpseLoot] 시체에 상호작용 영역을 붙일 Component가 없습니다."));
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = Monster;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    LootTargetActor = GetWorld()->SpawnActor<ABaruCorpseLootActor>(
        ABaruCorpseLootActor::StaticClass(), Monster->GetActorLocation(),
        FRotator::ZeroRotator, Params);
    if (!IsValid(LootTargetActor))
    {
        BARU_NET_LOG(Monster, LogBaruItem, Error,
            TEXT("[CorpseLoot] 상호작용 Actor 생성 실패."));
        return;
    }
    if (!LootTargetActor->AttachToComponent(AttachParent,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBone))
    {
        BARU_NET_LOG(Monster, LogBaruItem, Error,
            TEXT("[CorpseLoot] 상호작용 Actor 부착 실패."));
        LootTargetActor->Destroy();
        LootTargetActor = nullptr;
        return;
    }
    LootTargetActor->SetActorRelativeLocation(LootRelativeOffset);
    LootTargetActor->InitializeLootTarget(this, InteractionRadius);
}

bool UBaruMonsterItemSpawnerComponent::HasLoot() const
{
    return bLootGenerated && LootEntries.ContainsByPredicate(
        [](const FBaruCorpseLootEntry& Entry) { return Entry.Quantity > 0; });
}

UBaruInventoryComponent* UBaruMonsterItemSpawnerComponent::FindPlayerInventory(
    APawn* Interactor) const
{
    if (!IsValid(Interactor))
    {
        return nullptr;
    }
    APlayerState* PlayerState = Interactor->GetPlayerState();
    return IsValid(PlayerState)
        ? PlayerState->FindComponentByClass<UBaruInventoryComponent>() : nullptr;
}

bool UBaruMonsterItemSpawnerComponent::CanLoot(APawn* Interactor) const
{
    if (!IsValid(Interactor) || !HasLoot() || !IsDeadOwner() || bTransferInProgress)
    {
        return false;
    }
    if (Interactor->GetClass()->ImplementsInterface(UCombatInterface::StaticClass())
        && ICombatInterface::Execute_IsDead(Interactor))
    {
        return false;
    }
    return IsValid(FindPlayerInventory(Interactor));
}

int32 UBaruMonsterItemSpawnerComponent::LootAllOnServer(APawn* Interactor)
{
    AActor* Monster = GetOwner();
    if (!IsValid(Monster) || !Monster->HasAuthority() || !CanLoot(Interactor))
    {
        return 0;
    }
    UBaruInventoryComponent* Inventory = FindPlayerInventory(Interactor);
    if (!IsValid(Inventory) || Inventory->ItemDataTable != ItemDataTable.Get())
    {
        BARU_NET_LOG(Monster, LogBaruItem, Error,
            TEXT("[CorpseLoot] 플레이어 Inventory와 시체의 DT_Item이 다릅니다."));
        return 0;
    }

    int32 ReceivedTotal = 0;
    {
        // AddItem의 UI 알림 도중 같은 시체를 다시 습득하려는 재진입도 차단합니다.
        TGuardValue<bool> TransferGuard(bTransferInProgress, true);
        for (FBaruCorpseLootEntry& Entry : LootEntries)
        {
            if (Entry.Quantity <= 0)
            {
                continue;
            }
            const int32 Before = Entry.Quantity;
            // AddItem 반환값은 "획득 수량"이 아니라 "수납하지 못한 수량"입니다.
            const int32 Remaining = Inventory->AddItem(Entry.ItemID, Before);
            Entry.Quantity = FMath::Clamp(Remaining, 0, Before);
            const int32 Received = Before - Entry.Quantity;
            ReceivedTotal += Received;
            BARU_NET_LOG(Monster, LogBaruItem, Log,
                TEXT("[CorpseLoot] Item=%s 획득=%d 잔여=%d Player=%s"),
                *Entry.ItemID.ToString(), Received, Entry.Quantity, *GetNameSafe(Interactor));
        }
        LootEntries.RemoveAll(
            [](const FBaruCorpseLootEntry& Entry) { return Entry.Quantity <= 0; });
    }
    if (IsValid(Inventory->GetOwner()))
    {
        Inventory->GetOwner()->ForceNetUpdate();
    }
    NotifyLootUpdatedOnServer();
    return ReceivedTotal;
}

void UBaruMonsterItemSpawnerComponent::NotifyLootUpdatedOnServer()
{
    if (IsValid(LootTargetActor))
    {
        LootTargetActor->SetLootAvailable(HasLoot());
    }
    if (IsValid(GetOwner()))
    {
        GetOwner()->FlushNetDormancy();
        GetOwner()->ForceNetUpdate();
    }
    OnLootUpdated.Broadcast();
}

void UBaruMonsterItemSpawnerComponent::OnRep_LootState()
{
    OnLootUpdated.Broadcast();
}

void UBaruMonsterItemSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(GetOwner()) && GetOwner()->HasAuthority() && IsValid(LootTargetActor))
    {
        LootTargetActor->Destroy();
        LootTargetActor = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
