


#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
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
}



