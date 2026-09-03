#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "GameplayTags/BaruGameplayTags.h"
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
                if (AbilitySpec->IsActive()) AbilitySpecInputPressed(*AbilitySpec);
                else AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
            }
        }
    }

    // Held
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
    {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
        {
            if (AbilitySpec->Ability && !AbilitySpec->IsActive())
            {
                const UBaruGameplayAbility* BaruAbility = Cast<UBaruGameplayAbility>(AbilitySpec->Ability);
                if (BaruAbility && BaruAbility->GetActivationPolicy() == EBaruAbilityActivationPolicy::WhileInputActive)
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
        BARU_NET_LOG(GetAvatarActor(), LogBaruGAS, Warning, TEXT("TryActivateAbilityByTag Failed: Invalid Tag."));
        return false;
    }

    bool bSuccess = false;
    for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
    {
        if (!Spec.Ability)
        {
            continue;
        }
        
        // 고유 태그 검사
        const bool bHasAssetTag = Spec.Ability->GetAssetTags().HasTagExact(AbilityTag);
        
        // 런타임 동적 Spec 부여 태그 검사
        const bool bHasDynamicTag = Spec.GetDynamicSpecSourceTags().HasTagExact(AbilityTag);

        if (bHasAssetTag || bHasDynamicTag)
        {
            bSuccess |= TryActivateAbility(Spec.Handle);
        }
    }
    
    return bSuccess;
}




// Helper 함수

FActiveGameplayEffectHandle UBaruAbilitySystemComponent::ApplyDamageEffectToTarget(
    TSubclassOf<UGameplayEffect> DamageEffectClass,
    UAbilitySystemComponent* TargetASC,
    float PhysicalDamage,
    float SpecialDamage,
    float SuppressionDamage)
{
    if (!DamageEffectClass || !TargetASC)
    {
        BARU_NET_LOG(GetAvatarActor(), LogBaruGAS, Warning, TEXT("ApplyDamageEffectToTarget Failed: Invalid EffectClass or TargetASC."));
        return FActiveGameplayEffectHandle();
    }

    // Effect Context
    FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
    ContextHandle.AddInstigator(GetOwnerActor(), GetAvatarActor());

    // Spec
    const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
    if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
    {
        return FActiveGameplayEffectHandle();
    }

    
    // SetByCaller Magnitude
    
    // PhysicalDamage
    SpecHandle.Data->SetSetByCallerMagnitude(FBaruGameplayTags::Get().Data_Damage, PhysicalDamage);

    // SpecialDamage
    if (SpecialDamage > 0.0f)
    {
        SpecHandle.Data->SetSetByCallerMagnitude(FBaruGameplayTags::Get().Data_Damage_Special, SpecialDamage);
    }

    // SuppressionDamage
    if (SuppressionDamage >= 0.0f)
    {
        const FGameplayTag SuppressionTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Suppression"), false);
        if (SuppressionTag.IsValid())
        {
            SpecHandle.Data->SetSetByCallerMagnitude(SuppressionTag, SuppressionDamage);
        }
    }

    // 대상 ASC에 이펙트 적용
    return ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

FActiveGameplayEffectHandle UBaruAbilitySystemComponent::ApplyGenericEffectToTarget(
    TSubclassOf<UGameplayEffect> EffectClass,
    UAbilitySystemComponent* TargetASC,
    float Level)
{
    if (!EffectClass || !TargetASC)
    {
        return FActiveGameplayEffectHandle();
    }

    FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
    ContextHandle.AddInstigator(GetOwnerActor(), GetAvatarActor());

    const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, Level, ContextHandle);
    if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
    {
        return ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
    }

    return FActiveGameplayEffectHandle();
}