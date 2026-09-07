#include "AbilitySystem/Abilities/Monster/BaruGA_MonsterAttack.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "BaruLog.h"
#include "GameplayTags/BaruGameplayTags.h"

UBaruGA_MonsterAttack::UBaruGA_MonsterAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ActivationPolicy = EBaruAbilityActivationPolicy::OnInputTriggered;

    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Dead);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Debuff_Groggy);
}

void UBaruGA_MonsterAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (ACharacter* MonsterChar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (AttackMontage)
        {
            const float Duration = MonsterChar->PlayAnimMontage(AttackMontage);
            if (Duration <= 0.0f)
            {
                // 몽타주가 없거나 실패 시 즉시 타격 후 종료
                PerformMeleeAttackTrace();
                EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
                return;
            }
        }
        else
        {
            PerformMeleeAttackTrace();
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        }
    }
}

void UBaruGA_MonsterAttack::PerformMeleeAttackTrace()
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor || !HasAuthority(&CurrentActivationInfo))
    {
        return;
    }

    const FVector Start = AvatarActor->GetActorLocation();
    const FVector End = Start + (AvatarActor->GetActorForwardVector() * AttackRange);

    FCollisionQueryParams Params(TEXT("MonsterAttackTrace"), false, AvatarActor);
    FHitResult HitResult;

    // Sweep
    const bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(AttackRadius),
        Params
    );

    if (bHit && HitResult.GetActor())
    {
        AActor* HitActor = HitResult.GetActor();
        
        if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(HitActor))
        {
            if (UAbilitySystemComponent* TargetASC = CombatInterface->GetAbilitySystemComponent())
            {
                if (UBaruAbilitySystemComponent* MyASC = GetBaruAbilitySystemComponentFromActorInfo())
                {
                    MyASC->ApplyDamageEffectToTarget(DamageEffectClass, TargetASC, AttackDamage);
                    BARU_NET_LOG(AvatarActor, LogBaruCombat, Log, TEXT("Monster Hit Player: %s"), *HitActor->GetName());
                }
            }
        }
    }
}