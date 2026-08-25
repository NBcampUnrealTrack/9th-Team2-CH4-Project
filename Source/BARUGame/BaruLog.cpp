#include "BaruLog.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY(LogBaru);
DEFINE_LOG_CATEGORY(LogBaruNet);
DEFINE_LOG_CATEGORY(LogBaruGAS);
DEFINE_LOG_CATEGORY(LogBaruCombat);
DEFINE_LOG_CATEGORY(LogBaruAI);
DEFINE_LOG_CATEGORY(LogBaruItem);
DEFINE_LOG_CATEGORY(LogBaruUI);
DEFINE_LOG_CATEGORY(LogBaruSession);
DEFINE_LOG_CATEGORY(LogBaruSanity);
DEFINE_LOG_CATEGORY(LogBaruBackend);

namespace BaruLog
{
	FString GetNetPrefix(const UObject* WorldContext)
	{
		if (!WorldContext)
		{
			return TEXT("[STATIC]");
		}

		FString NetPrefix = TEXT("[UNKNOWN]");
		FString RolePrefix = TEXT("[NoRole]");
		FString ActorName = TEXT("None");

		if (const UWorld* World = WorldContext->GetWorld())
		{
			NetPrefix = World->IsNetMode(NM_DedicatedServer) ? TEXT("[SERVER]") :
						World->IsNetMode(NM_Client) ? TEXT("[CLIENT]") :
						World->IsNetMode(NM_ListenServer) ? TEXT("[LISTEN_SERVER]") : TEXT("[STANDALONE]");
		}

		if (const AActor* Actor = Cast<AActor>(WorldContext))
		{
			ActorName = Actor->GetName();
			const ENetRole Role = Actor->GetLocalRole();
			RolePrefix = (Role == ROLE_Authority) ? TEXT("[Authority]") :
						 (Role == ROLE_AutonomousProxy) ? TEXT("[Autonomous]") :
						 (Role == ROLE_SimulatedProxy) ? TEXT("[Simulated]") : TEXT("[None]");
		}

		return FString::Printf(TEXT("%s%s[%s]"), *NetPrefix, *RolePrefix, *ActorName);
	}
}