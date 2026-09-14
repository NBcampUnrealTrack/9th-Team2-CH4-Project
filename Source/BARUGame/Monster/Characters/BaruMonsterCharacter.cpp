


#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "Monster/AI/BaruMonsterDirector.h"

#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "AbilitySystem/Attributes/BaruMonsterAttributeSet.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "GameFramework/Controller.h"
#include "Core/BaruGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "BrainComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "BaruLog.h"


ABaruMonsterCharacter::ABaruMonsterCharacter()
{
	
	PrimaryActorTick.bCanEverTick = false;
	
	// 서버가 확정한 몬스터의 상태를 클라이언트에도 전달
	bReplicates = true;
	
	// 서버에서 이동한 몬스터의 위치와 회전을 접속한 플레이어들의 화면에도 동기화
	SetReplicateMovement(true);
	
	// 이 몬스터를 조종할 AIController를 지정
	AIControllerClass = ABaruMonsterAIController::StaticClass();
	
	// 맵에 직접 배치되거나 게임 중 생성된 몬스터 모두 자동으로 AIController의 조종을 받도록 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	// 모든 몬스터가 자신의 ASC 인스턴스를 가지도록 생성
	AbilitySystemComponent = CreateDefaultSubobject<UBaruAbilitySystemComponent>(
			TEXT("AbilitySystemComponent"));
	
	// 체력, 방어력, 이동속도를 보관하는 공용 AttributeSet 생성
	CoreAttributeSet = CreateDefaultSubobject<UBaruCoreAttributeSet>(
			TEXT("CoreAttributeSet"));
	
	// 제압 게이지와 제압 피해를 보관하는 몬스터 전용 AttributeSet 생성
	MonsterAttributeSet = CreateDefaultSubobject<UBaruMonsterAttributeSet>(
			TEXT("MonsterAttributeSet"));
	
}

void ABaruMonsterCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	// 부모 Character가 사용하는 복제 설정도 함께 등록
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 서버의 bIsDead 값이 변경되면 클라이언트로 복제
	// 클라이언트에서는 값이 도착한 뒤 OnRep_IsDead가 자동 호출됨
	DOREPLIFETIME(
		ABaruMonsterCharacter,
		bIsDead
	);
}

void ABaruMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 서버에서 생성되거나 맵에 배치된 몬스터를
	// GameMode의 생존 몬스터 목록에 등록
	if (HasAuthority())
	{
		if (ABaruGameMode* BaruGameMode =
			GetWorld()->GetAuthGameMode<ABaruGameMode>())
		{
			BaruGameMode->RegisterMonster(this);
		}
	}
	
	// 몬스터가 실제 게임 월드에 들어왔는지 확인하기 위한 로그
	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT("Monster Character BeginPlay")
	);
	
	// 서버에서 설정표가 빠진 몬스터를 발견하면 경고를 출력
	if (HasAuthority() && !IsValid(MonsterDataAsset))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Warning,
			TEXT("Monster DataAsset is not assigned.")
		);
	}
	
	// ASC와 두 AttributeSet 중 하나라도 생성되지 않았다면 초기화 중단
	if (!IsValid(AbilitySystemComponent) ||
		!IsValid(CoreAttributeSet) ||
		!IsValid(MonsterAttributeSet))
	{
		BARU_NET_LOG(
			this,
			LogBaruGAS,
			Error,
			TEXT(
				"Monster ASC or AttributeSets are invalid."
			)
		);

		return;
	}
	
	// 위에서 유효성을 확인했으므로 바로 초기화
	// 몬스터는 ASC의 소유자와 실제 몸이 모두 자기 자신
	AbilitySystemComponent->InitAbilityActorInfo(
		this,
		this
		);
	
	// ASC 초기화가 끝난 뒤 DataAsset의 초기 능력치를 적용
	ApplyInitialAttributesFromDataAsset();
	
	// ASC 초기화가 끝난 뒤 몬스터의 초기 Ability를 등록
	GrantInitialAbilities();
		
	// 생성한 AttributeSet이 ASC에 등록됐는지 확인
	const UBaruCoreAttributeSet* RegisteredCoreAttributeSet =
		AbilitySystemComponent->GetSet<UBaruCoreAttributeSet>();
	
	// 몬스터 전용 AttributeSet이 ASC에 등록됐는지 확인
	const UBaruMonsterAttributeSet* RegisteredMonsterAttributeSet =
		AbilitySystemComponent->GetSet<UBaruMonsterAttributeSet>();
	
	// 등록 여부를 먼저 검사한 뒤 사용
	if (!IsValid(RegisteredCoreAttributeSet) ||
		!IsValid(RegisteredMonsterAttributeSet))
	{
		BARU_NET_LOG(
			this,
			LogBaruGAS,
			Error,
			TEXT(
				"Monster AttributeSets are not registered."
			)
		);

		return;
	}
	
	// Core MoveSpeed가 변경될 때마다 실제 캐릭터 이동속도를 갱신
	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(UBaruCoreAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ABaruMonsterCharacter::HandleMoveSpeedAttributeChanged);

	// 델리게이트 연결 전에 이미 설정돼 있던 초기 이동속도도 한 번 반영
	UCharacterMovementComponent* MovementComponent =
		GetCharacterMovement();

	if (!IsValid(MovementComponent))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Error,
			TEXT(
				"Monster CharacterMovementComponent "
				"is invalid."
			)
		);

		return;
	}

	MovementComponent->MaxWalkSpeed =
		FMath::Max(0.0f, RegisteredCoreAttributeSet->GetMoveSpeed());
	

	// 초기화 확인용 로그
	BARU_NET_LOG(
		this,
		LogBaruGAS,
		Log,
		TEXT(
			"Monster GAS initialized. "
			"Health=%.1f, Suppression=%.1f"
		),
		RegisteredCoreAttributeSet->GetHealth(),
		RegisteredMonsterAttributeSet->GetSuppression()
	);
	
	// 그로기 판정과 회복 타이머는 서버에서만 관리
	if (HasAuthority())
	{
		const FGameplayTag GroggyTag =
			FBaruGameplayTags::Get().State_Debuff_Groggy;

		// 태그가 처음 생기거나 완전히 사라질 때 알림을 받음
		GroggyTagChangedHandle =
			AbilitySystemComponent->RegisterGameplayTagEvent(
				GroggyTag,
				EGameplayTagEventType::NewOrRemoved
			).AddUObject(
				this,
				&ABaruMonsterCharacter::HandleGroggyTagChanged
			);

		// 연결 전에 이미 그로기 태그가 있었다면 현재 상태도 반영
		HandleGroggyTagChanged(
			GroggyTag,
			AbilitySystemComponent->GetTagCount(GroggyTag)
		);
	}
	
	// 디렉터 검색과 등록은 서버에서만 처리
	if (HasAuthority() && bUsesMonsterDirector)
	{
		// 레벨 또는 스폰 과정에서 디렉터가 직접 지정되지 않았다면
		// 현재 월드에 배치된 첫 번째 몬스터 디렉터를 자동으로 찾음
		if (!IsValid(AssignedDirector))
		{
			AssignedDirector =
				Cast<ABaruMonsterDirector>(
					UGameplayStatics::GetActorOfClass(
						GetWorld(),
						ABaruMonsterDirector::StaticClass()
					)
				);
		}

		// 직접 지정됐거나 자동으로 찾은 디렉터에
		// 현재 몬스터 자신을 지휘 대상으로 등록
		if (IsValid(AssignedDirector))
		{
			AssignedDirector->RegisterMonster(this);
		}
	}
	
}

void ABaruMonsterCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	// 사망 외에 강제 삭제나 레벨 종료로 제거되는 경우에도 등록 해제
	// 사망 시 이미 해제했더라도 중복 호출로 다시 추가되지는 않음
	if (HasAuthority() && IsValid(AssignedDirector))
	{
		AssignedDirector->UnregisterMonster(this);
	}
	
	// 몬스터가 제거된 뒤 회복 함수가 실행되지 않도록 예약 취소
	GetWorldTimerManager().ClearTimer(GroggyRecoveryTimerHandle);

	// ASC에서 받던 그로기 태그 변경 알림 연결 해제
	if (IsValid(AbilitySystemComponent) &&
		GroggyTagChangedHandle.IsValid())
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(
			FBaruGameplayTags::Get().State_Debuff_Groggy,
			EGameplayTagEventType::NewOrRemoved
		).Remove(GroggyTagChangedHandle);

		GroggyTagChangedHandle.Reset();
	}
	
	// 정상적인 사망이 아닌 맵 이탈, 강제 삭제 등의 이유로
	// 몬스터가 사라진 경우 GameMode 목록에서도 제거
	if (HasAuthority() && !bIsDead)
	{
		if (ABaruGameMode* BaruGameMode =
			GetWorld()->GetAuthGameMode<ABaruGameMode>())
		{
			BaruGameMode->UnregisterMonster(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

//몬스터 블루프린트에서 지정한 DataAsset을 읽을 때 사용
//설정표가 지정되지 않았다면 nullptr를 반환
const UBaruMonsterDataAsset* ABaruMonsterCharacter::GetMonsterDataAsset() const
{
	// TObjectPtr에 보관된 설정표를 읽기 전용 포인터로 꺼내 반환
	return MonsterDataAsset.Get();
}

// 이 몬스터가 사용하는 ASC를 공용 인터페이스로 반환
UAbilitySystemComponent* ABaruMonsterCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

void ABaruMonsterCharacter::
	HandleMoveSpeedAttributeChanged(
		const FOnAttributeChangeData& AttributeChangeData
	)
{
	UCharacterMovementComponent* MovementComponent =
		GetCharacterMovement();

	if (!IsValid(MovementComponent))
	{
		return;
	}

	// GAS가 계산한 최종 MoveSpeed를 실제 이동 컴포넌트에 반영
	const float NewMoveSpeed = FMath::Max(
		0.0f,
		AttributeChangeData.NewValue
	);

	MovementComponent->MaxWalkSpeed = NewMoveSpeed;

	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT(
			"Monster MoveSpeed changed. "
			"Old=%.1f, New=%.1f"
		),
		AttributeChangeData.OldValue,
		NewMoveSpeed
	);
}

void ABaruMonsterCharacter::ApplyInitialAttributesFromDataAsset()
{
    // 능력치의 최초 설정은 서버에서만 처리
    if (!HasAuthority() ||
        !IsValid(AbilitySystemComponent) ||
        !IsValid(MonsterDataAsset))
    {
        return;
    }

    // DataAsset 값을 안전한 범위로 보정
    const float InitialMaxHealth =
        FMath::Max(1.0f, MonsterDataAsset->MaxHealth);

    const float InitialPhysicalDefense =
        FMath::Max(0.0f, MonsterDataAsset->PhysicalDefense);

    const float InitialSpecialResistance =
        FMath::Clamp(
            MonsterDataAsset->SpecialResistance,
            0.0f,
            1.0f
        );

    const float InitialMaxSuppression =
        FMath::Max(1.0f, MonsterDataAsset->MaxSuppression);

    const float InitialMoveSpeed =
        FMath::Max(0.0f, MonsterDataAsset->PatrolSpeed);

    // 최대 체력을 먼저 설정한 뒤 현재 체력을 가득 채움
    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruCoreAttributeSet::GetMaxHealthAttribute(),
        InitialMaxHealth
    );

    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruCoreAttributeSet::GetHealthAttribute(),
        InitialMaxHealth
    );

    // 방어 능력치 적용
    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruCoreAttributeSet::GetPhysicalDefenseAttribute(),
        InitialPhysicalDefense
    );

    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruCoreAttributeSet::GetSpecialResistanceAttribute(),
        InitialSpecialResistance
    );

    // 시작 이동속도는 순찰 속도로 설정
    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruCoreAttributeSet::GetMoveSpeedAttribute(),
        InitialMoveSpeed
    );

    // 최대 제압도를 먼저 설정한 뒤 현재 제압도를 가득 채움
    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruMonsterAttributeSet::GetMaxSuppressionAttribute(),
        InitialMaxSuppression
    );

    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruMonsterAttributeSet::GetSuppressionAttribute(),
        InitialMaxSuppression
    );

    BARU_NET_LOG(
        this,
        LogBaruGAS,
        Log,
        TEXT(
            "Monster initial attributes applied. "
            "Health=%.1f, Defense=%.1f, "
            "Resistance=%.2f, Suppression=%.1f"
        ),
        InitialMaxHealth,
        InitialPhysicalDefense,
        InitialSpecialResistance,
        InitialMaxSuppression
    );
}

void ABaruMonsterCharacter::GrantInitialAbilities()
{
	// Ability 부여는 서버에서만 실행
	if (!HasAuthority() ||
		!IsValid(AbilitySystemComponent) ||
		!IsValid(MonsterDataAsset) ||
		!MonsterDataAsset->AttackAbilityClass)
	{
		return;
	}

	// DataAsset에 지정된 공격 Ability를 ASC에 등록
	AbilitySystemComponent->GiveAbility(
		FGameplayAbilitySpec(
			MonsterDataAsset->AttackAbilityClass,
			1
		)
	);
}

//---------
//Dead 관련
//---------

void ABaruMonsterCharacter::Die_Implementation(AActor* Killer)
{
	// 사망 판정은 서버에서만 처리하며 중복 실행을 막음
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;
	
	// 사망이 확정된 몬스터를 디렉터의 지휘 목록에서 제외
	// bIsDead를 먼저 설정했으므로 살아 있는 개체로 처리되지 않음
	//
	// 시체 액터는 삭제하지 않으며 기존 사망 연출은 이어서 실행
	if (IsValid(AssignedDirector))
	{
		AssignedDirector->UnregisterMonster(this);
	}
	
	// 그로기 도중 사망하면 회복 예약 취소
	// bIsDead를 먼저 설정해서 이후에도 행동이 재개되지 않도록 함
	GetWorldTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
	bIsGroggy = false;

	// 서버 화면에도 즉시 사망 상태를 적용
	// 클라이언트에서는 bIsDead가 복제될 때 자동 호출됨
	OnRep_IsDead();
	
	// GameMode에 죽은 몬스터와 Killer를 전달
	// GameMode는 몬스터 수와 플레이어의 킬 정보를 갱신
	if (ABaruGameMode* BaruGameMode =
		GetWorld()->GetAuthGameMode<ABaruGameMode>())
	{
		BaruGameMode->OnMonsterDied(this, Killer);
	}

	BARU_NET_LOG(
		this,
		LogBaruCombat,
		Log,
		TEXT("Monster died. Killer=%s"),
		*GetNameSafe(Killer)
	);
}

bool ABaruMonsterCharacter::IsDead_Implementation() const
{
	return bIsDead;
}

void ABaruMonsterCharacter::OnRep_IsDead()
{
	if (!bIsDead)
	{
		return;
	}

	// 진행 중인 이동을 멈추고 이후 이동도 차단
	if (UCharacterMovementComponent* MovementComponent =
		GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	// 실행 중인 공격 Ability를 모두 중단
	// 이후 State.Dead에 의해 공격이 다시 실행되지 않도록 함
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();
		
		const FGameplayTag DeadTag =
	   FBaruGameplayTags::Get().State_Dead;

		if (DeadTag.IsValid() &&
			!AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
		{
			AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
		}
	}

	// AIController가 요청한 현재 이동을 멈춤
	if (ABaruMonsterAIController* MonsterController =
		Cast<ABaruMonsterAIController>(GetController()))
	{
		MonsterController->StopMovement();

		// Behavior Tree의 판단과 Task 실행도 완전히 정지
		if (UBrainComponent* BrainComponent =
			MonsterController->GetBrainComponent())
		{
			BrainComponent->StopLogic(TEXT("Monster died"));
		}
	}
	
	// 사망한 몬스터의 캡슐 충돌을 꺼서
	// 플레이어와 AI의 이동을 막지 않도록 함
	if (UCapsuleComponent* MonsterCapsule = GetCapsuleComponent())
	{
		MonsterCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 각 몬스터 블루프린트에 구현된 사망 연출 실행
	// 래그돌을 켜더라도 액터를 삭제하지 않으므로 시체는 유지됨
	OnDeathCosmetic();
	
}

//----------------
// Groggy 관련
//----------------

void ABaruMonsterCharacter::HandleGroggyTagChanged(
    const FGameplayTag Tag,
    int32 NewCount
)
{
    // 상태 변경은 서버에서만 처리
    // 사망한 몬스터는 그로기 진입과 행동 재개 모두 차단
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    // 이 함수가 처리할 태그인지 확인
    if (Tag != FBaruGameplayTags::Get().State_Debuff_Groggy)
    {
        return;
    }

    if (NewCount > 0)
    {
        EnterGroggy();
    }
    else
    {
        ExitGroggy();
    }
}

void ABaruMonsterCharacter::EnterGroggy()
{
    // 중복 진입으로 회복 시간이 계속 초기화되는 것을 방지
    if (!HasAuthority() || bIsDead || bIsGroggy)
    {
        return;
    }

    bIsGroggy = true;

    // Behavior Tree를 중단해서 새로운 행동 요청을 멈춤
    if (ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(GetController()))
    {
        if (UBrainComponent* MonsterBrain =
            MonsterController->GetBrainComponent())
        {
            MonsterBrain->StopLogic(TEXT("Monster entered groggy"));
        }

        // 현재 목적지로 이동하던 요청도 취소
        MonsterController->StopMovement();
    }

    // 이미 재생 중인 공격 Ability도 취소
    // 새 공격은 기존 ActivationBlockedTags의 그로기 태그로 차단
    if (IsValid(AbilitySystemComponent))
    {
        AbilitySystemComponent->CancelAllAbilities();
    }

    // 이동 속도를 즉시 없애고 이동 모드를 비활성화
    if (UCharacterMovementComponent* MovementComponent =
        GetCharacterMovement())
    {
        MovementComponent->StopMovementImmediately();
        MovementComponent->DisableMovement();
    }

    // Ability 취소 과정에서 사망하거나 그로기가 해제됐으면 예약하지 않음
    if (bIsDead || !bIsGroggy)
    {
        return;
    }

    // DataAsset이 없으면 기본 5초 사용
    const float RecoveryDelay = IsValid(MonsterDataAsset)
        ? FMath::Max(0.1f, MonsterDataAsset->GroggyDuration)
        : 5.0f;

    // 지정된 시간이 지나면 한 번만 회복 함수 실행
    GetWorldTimerManager().SetTimer(
        GroggyRecoveryTimerHandle,
        this,
        &ABaruMonsterCharacter::RecoverFromGroggy,
        RecoveryDelay,
        false
    );

    BARU_NET_LOG(
        this,
        LogBaruCombat,
        Log,
        TEXT("Monster groggy started. Duration=%.1f"),
        RecoveryDelay
    );
}

void ABaruMonsterCharacter::RecoverFromGroggy()
{
    // 사망한 몬스터나 이미 그로기가 끝난 몬스터는 회복하지 않음
    if (!HasAuthority() || bIsDead || !bIsGroggy ||
        !IsValid(AbilitySystemComponent) ||
        !IsValid(MonsterAttributeSet))
    {
        return;
    }

    // 현재 최대 제압도까지 회복
    const float RecoverySuppression =
        FMath::Max(0.0f, MonsterAttributeSet->GetMaxSuppression());

    AbilitySystemComponent->SetNumericAttributeBase(
        UBaruMonsterAttributeSet::GetSuppressionAttribute(),
        RecoverySuppression
    );

    // AttributeSet에서 추가했던 Loose 태그 1개를 제거
    // 태그 개수가 0이 되면 HandleGroggyTagChanged → ExitGroggy 호출
    AbilitySystemComponent->RemoveLooseGameplayTag(
        FBaruGameplayTags::Get().State_Debuff_Groggy
    );
}

void ABaruMonsterCharacter::ExitGroggy()
{
    // 사망한 몬스터의 이동과 AI가 다시 켜지는 것을 방지
    if (!HasAuthority() || bIsDead || !bIsGroggy)
    {
        return;
    }

    bIsGroggy = false;

    // 다른 시스템에서 먼저 태그를 제거한 경우에도 남은 예약 취소
    GetWorldTimerManager().ClearTimer(GroggyRecoveryTimerHandle);

    // 현재 지상 보행 몬스터 기준으로 이동 재개
    // 공중에 있다면 낙하 모드로 복귀
    if (UCharacterMovementComponent* MovementComponent =
        GetCharacterMovement())
    {
        MovementComponent->SetDefaultMovementMode();
    }

    BARU_NET_LOG(
        this,
        LogBaruCombat,
        Log,
        TEXT("Monster groggy ended.")
    );

    // 이동이 가능한 상태로 복구한 뒤 Behavior Tree를 다시 시작
    // 현재 Blackboard 값을 기준으로 다음 행동을 판단
    if (ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(GetController()))
    {
        if (UBrainComponent* MonsterBrain =
            MonsterController->GetBrainComponent())
        {
            MonsterBrain->RestartLogic();
        }
    }
}

//----------------
// 피격 반응
//----------------

void ABaruMonsterCharacter::ApplyCombatDamage_Implementation(
    float DamageAmount,
    const FHitResult& HitResult,
    AActor* DamageCauser,
    AController* InstigatedBy
)
{
    // 체력은 이미 처리됐으므로 위협도와 밀림만 처리
    if (!HasAuthority() || bIsDead ||
        !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
    {
        return;
    }

    // 공격자가 PlayerState라면 ASC에 연결된 실제 몸을 사용
    AActor* AttackOriginActor = DamageCauser;

    if (IsValid(DamageCauser))
    {
        if (IAbilitySystemInterface* SourceInterface =
            Cast<IAbilitySystemInterface>(DamageCauser))
        {
            if (UAbilitySystemComponent* SourceASC =
                SourceInterface->GetAbilitySystemComponent())
            {
                if (AActor* SourceAvatar = SourceASC->GetAvatarActor())
                {
                    AttackOriginActor = SourceAvatar;
                }
            }
        }

        // Controller가 전달됐다면 조종 중인 Pawn 사용
        if (AController* SourceController =
            Cast<AController>(AttackOriginActor))
        {
            AttackOriginActor = SourceController->GetPawn();
        }
    }

    if (!IsValid(AttackOriginActor) && IsValid(InstigatedBy))
    {
        AttackOriginActor = InstigatedBy->GetPawn();
    }

    if (!IsValid(AttackOriginActor) || AttackOriginActor == this)
    {
        return;
    }

    // [추가] 그로기·밀림 검사보다 먼저 피해 위협도 등록
    if (ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(GetController()))
    {
        if (APawn* AttackerPawn = Cast<APawn>(AttackOriginActor))
        {
            MonsterController->RegisterDamageThreat(
                AttackerPawn,
                DamageAmount
            );
        }
    }

    // 그로기 중에는 위협도만 등록하고 밀림 생략
    if (bIsGroggy ||
        (IsValid(AbilitySystemComponent) &&
         AbilitySystemComponent->HasMatchingGameplayTag(
             FBaruGameplayTags::Get().State_Debuff_Groggy
         )))
    {
        return;
    }

    UCharacterMovementComponent* MovementComponent =
        GetCharacterMovement();

    // 이동이 비활성화된 상태는 그대로 유지
    if (!IsValid(MovementComponent) ||
        MovementComponent->MovementMode == MOVE_None)
    {
        return;
    }

    const float PushSpeed = IsValid(MonsterDataAsset)
        ? FMath::Max(0.0f, MonsterDataAsset->HitPushSpeed)
        : 200.0f;

    if (PushSpeed <= 0.0f)
    {
        return;
    }

    // 공격자의 반대 방향으로 수평 밀림
    const FVector PushDirection =
        (GetActorLocation() - AttackOriginActor->GetActorLocation())
        .GetSafeNormal2D();

    if (PushDirection.IsNearlyZero())
    {
        return;
    }

    MovementComponent->StopMovementImmediately();

    MovementComponent->AddImpulse(
        PushDirection * PushSpeed,
        true
    );

    BARU_NET_LOG(
        this,
        LogBaruCombat,
        Log,
        TEXT("Monster hit push. Source=%s, Speed=%.1f"),
        *GetNameSafe(AttackOriginActor),
        PushSpeed
    );
}