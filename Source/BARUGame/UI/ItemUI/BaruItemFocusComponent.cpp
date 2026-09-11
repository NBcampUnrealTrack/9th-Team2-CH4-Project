#include "UI/ItemUI/BaruItemFocusComponent.h"

#include "BaruLog.h"
#include "Character/BaruCharacter.h"
#include "Core/BaruGameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/Items/BaruBaseItem.h"
#include "Interfaces/InteractableInterface.h"
#include "Player/BaruPlayerState.h"
#include "UI/ItemUI/BaruItemFocusWidget.h"

UBaruItemFocusComponent::UBaruItemFocusComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
    SetIsReplicatedByDefault(false);
}

void UBaruItemFocusComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!Cast<APlayerController>(GetOwner()))
    {
        BARU_LOG(LogBaruUI, Warning,
            TEXT("BaruItemFocusComponent: PlayerController BP에 추가해야 합니다."));
        SetComponentTickEnabled(false);
    }
    else if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        SetComponentTickEnabled(false);
    }
}

void UBaruItemFocusComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!IsValid(PC) || !PC->IsLocalController()) return;

    if (!IsValid(FocusWidget))
    {
        FocusWidget = CreateWidget<UBaruItemFocusWidget>(
            PC, UBaruItemFocusWidget::StaticClass());
        if (!IsValid(FocusWidget)) return;
        if (!FocusWidget->AddToPlayerScreen(5))
        {
            FocusWidget = nullptr;
            return;
        }
    }
    FocusWidget->ConfigureCard(CardAnchor, CardWidth);

    ABaruCharacter* Character = Cast<ABaruCharacter>(PC->GetPawn());
    const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>();
    const ABaruGameState* GS = GetWorld()->GetGameState<ABaruGameState>();
    if (!bEnableItemFocusUI || PC->bShowMouseCursor
        || !IsValid(Character) || !IsValid(PS) || !PS->IsAlive()
        || (IsValid(GS) && GS->GetMatchState() == EBaruMatchState::PostGame))
    {
        FocusWidget->HideItem();
        return;
    }

    // 실제 상호작용과 동일한 Character의 카메라/채널 탐색을 재사용합니다.
    FHitResult Hit;
    if (!FMath::IsFinite(FocusTraceDistance) || FocusTraceDistance <= 0.0f
        || !Character->PerformLineTrace(Hit, FocusTraceDistance, false))
    {
        FocusWidget->HideItem();
        return;
    }

    ABaruBaseItem* Item = Cast<ABaruBaseItem>(Hit.GetActor());
    if (!IsValid(Item) || !IInteractableInterface::Execute_CanInteract(Item, Character))
    {
        FocusWidget->HideItem();
        return;
    }
    FocusWidget->ShowItem(Item, Character);
}

void UBaruItemFocusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(FocusWidget)) FocusWidget->RemoveFromParent();
    FocusWidget = nullptr;
    Super::EndPlay(EndPlayReason);
}
