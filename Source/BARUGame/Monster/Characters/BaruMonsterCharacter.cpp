


#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
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
	
}


void ABaruMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 몬스터가 실제 게임 월드에 들어왔는지 확인하기 위한 로그
	BARU_NET_LOG(
		this,
		LogBaruAI,
		Verbose,
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
	
	if (IsValid(AbilitySystemComponent))
	{
		// 몬스터는 ASC의 소유자와 실제 몸이 모두 자기 자신
		AbilitySystemComponent->InitAbilityActorInfo(
			this,
			this
		);
	}
	
}

//몬스터 블루프린트에서 지정한 DataAsset을 읽을 때 사용
//설정표가 지정되지 않았다면 nullptr를 반환
const UBaruMonsterDataAsset* ABaruMonsterCharacter::GetMonsterDataAsset() const
{
	// TObjectPtr에 보관된 설정표를 읽기 전용 포인터로 꺼내 반환
	return MonsterDataAsset.Get();
}

UAbilitySystemComponent* ABaruMonsterCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}


