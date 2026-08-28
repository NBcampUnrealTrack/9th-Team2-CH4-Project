


#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Data/BaruMonsterDataAsset.h"

#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "AbilitySystem/Attributes/BaruMonsterAttributeSet.h"

#include "GameFramework/CharacterMovementComponent.h"
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


void ABaruMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
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


