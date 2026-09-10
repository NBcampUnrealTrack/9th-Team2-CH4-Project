#include "AbilitySystem/Abilities/Monster/BaruGA_MonsterAttack.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
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
    Super::ActivateAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        TriggerEventData
    );
    
    // 새로운 공격이 시작됐으므로 타격 여부 초기화
    bAttackHitProcessed = false;

    // 비용이나 쿨다운 적용에 실패하면 Ability를 취소
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true
        );

        return;
    }

    // 공격 몽타주가 지정되지 않았다면
    // 기존 방식대로 즉시 타격하고 Ability 종료
    if (!IsValid(AttackMontage))
    {
        PerformMeleeAttackTrace();

        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            false
        );

        return;
    }

    // 공격 몽타주를 재생하고 종료될 때까지 기다리는 Ability Task 생성
    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::
        CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage
        );

    if (!IsValid(MontageTask))
    {
        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true
        );

        return;
    }

    // 몽타주가 정상적으로 끝나면 Ability도 정상 종료
    MontageTask->OnCompleted.AddDynamic(
        this,
        &UBaruGA_MonsterAttack::HandleAttackMontageCompleted
    );

    // 몽타주가 다른 동작에 의해 중단된 경우
    MontageTask->OnInterrupted.AddDynamic(
        this,
        &UBaruGA_MonsterAttack::HandleAttackMontageCancelled
    );

    // Ability 취소 등으로 몽타주가 취소된 경우
    MontageTask->OnCancelled.AddDynamic(
        this,
        &UBaruGA_MonsterAttack::HandleAttackMontageCancelled
    );

    // 몬스터 AnimInstance에서 발생하는 Montage Notify 신호를 수신
    if (ActorInfo)
    {
        if (UAnimInstance* AnimInstance =
            ActorInfo->GetAnimInstance())
        {
            AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(
                this,
                &UBaruGA_MonsterAttack::HandleAttackMontageNotifyBegin
            );
        }
    }
    
    // Ability Task 실행 시작
    MontageTask->ReadyForActivation();
}

void UBaruGA_MonsterAttack::HandleAttackMontageCompleted()
{
    // 이미 종료된 Ability라면 중복 종료하지 않음
    if (!IsActive())
    {
        return;
    }
    
    // 몽타주가 정상적으로 끝났으므로 Ability 정상 종료
    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        false
    );
}

void UBaruGA_MonsterAttack::HandleAttackMontageCancelled()
{
    // 이미 종료된 Ability라면 중복 종료하지 않음
    if (!IsActive())
    {
        return;
    }

    // 몽타주가 중단되었으므로 Ability 취소 상태로 종료
    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        true
    );
}

void UBaruGA_MonsterAttack::HandleAttackMontageNotifyBegin(
    FName NotifyName,
    const FBranchingPointNotifyPayload& BranchingPointPayload
)
{
    // 현재는 Payload 내용을 사용하지 않음
    (void)BranchingPointPayload;

    // 다른 Montage Notify는 공격 판정에 사용하지 않음
    if (NotifyName != TEXT("AttackHit"))
    {
        return;
    }

    // Ability가 이미 끝났거나 이번 공격의 타격을 처리했다면 중단
    if (!IsActive() || bAttackHitProcessed)
    {
        return;
    }

    // 같은 공격에서 피해가 여러 번 들어가는 것을 방지
    bAttackHitProcessed = true;

    // Notify가 실행된 현재 순간의 위치로 실제 공격 판정
    PerformMeleeAttackTrace();
}

void UBaruGA_MonsterAttack::PerformMeleeAttackTrace()
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor || !HasAuthority(&CurrentActivationInfo))
    {
        return;
    }

    // DataAsset을 읽을 수 없는 경우 Ability의 기존 공격 거리를 사용
    float EffectiveAttackRange = FMath::Max(0.0f, AttackRange);

    // 공격하는 몬스터의 DataAsset에서 실제 타격 거리를 가져옴
    if (const ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(AvatarActor))
    {
        if (const UBaruMonsterDataAsset* MonsterData =
            MonsterCharacter->GetMonsterDataAsset())
        {
            EffectiveAttackRange =
                FMath::Max(0.0f, MonsterData->AttackRange);
        }
    }

    // 몬스터 위치에서 정면으로 설정된 거리만큼 구를 이동시켜 검사
    const FVector Start = AvatarActor->GetActorLocation();

    const FVector End =
        Start +
        AvatarActor->GetActorForwardVector() * EffectiveAttackRange;

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

void UBaruGA_MonsterAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled
)
{
    // 다음 공격에서 중복 호출되지 않도록 Notify 연결 해제
    if (ActorInfo)
    {
        if (UAnimInstance* AnimInstance =
            ActorInfo->GetAnimInstance())
        {
            AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(
                this,
                &UBaruGA_MonsterAttack::HandleAttackMontageNotifyBegin
            );
        }
    }

    Super::EndAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        bReplicateEndAbility,
        bWasCancelled
    );
}
