#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruLog.h"

UBaruAbilitySystemComponent::UBaruAbilitySystemComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicated(true);
}

void UBaruAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
    if (!InputTag.IsValid())
    {
        return;
    }
    
    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
    {
        if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) || AbilitySpec.Ability->GetAssetTags().HasTagExact(InputTag)))
        {
            InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
            InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
        }
    }
}

void UBaruAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
    if (!InputTag.IsValid())
    {
        return;
    }

    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
    {
        if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) || AbilitySpec.Ability->GetAssetTags().HasTagExact(InputTag)))
        {
            InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
            InputHeldSpecHandles.Remove(AbilitySpec.Handle);
        }
    }
}

void UBaruAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
    if (bGamePaused)
    {
        return;
    }

    TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

    // Pressed
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
    {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
        {
            if (AbilitySpec->Ability)
            {
                AbilitySpec->InputPressed = true;
                if (AbilitySpec->IsActive())
                {
                    AbilitySpecInputPressed(*AbilitySpec);
                }
                else
                {
                    AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                }
            }
        }
    }

    // Activate
    for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
    {
        TryActivateAbility(AbilitySpecHandle);
    }

    // Release
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
    {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
        {
            if (AbilitySpec->Ability)
            {
                AbilitySpec->InputPressed = false;
                if (AbilitySpec->IsActive())
                {
                    AbilitySpecInputReleased(*AbilitySpec);
                }
            }
        }
    }

    InputPressedSpecHandles.Reset();
    InputReleasedSpecHandles.Reset();
}

void UBaruAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReasonTags)
{
    Super::NotifyAbilityFailed(Handle, Ability, FailureReasonTags);

    BARU_NET_LOG(GetAvatarActor(), LogBaruGAS, Warning, TEXT("Ability Activation Failed: %s, Reason Tags: %s"), 
        Ability ? *Ability->GetName() : TEXT("None"), 
        *FailureReasonTags.ToStringSimple());
    
    if (OnAbilityActivationFailed.IsBound())
    {
        OnAbilityActivationFailed.Broadcast(Ability, FailureReasonTags);
    }
}

bool UBaruAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
    if (!AbilityTag.IsValid())
    {
        return false;
    }

    bool bSuccess = false;
    for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
    {
        if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(AbilityTag))
        {
            bSuccess |= TryActivateAbility(Spec.Handle);
        }
    }
    return bSuccess;
}