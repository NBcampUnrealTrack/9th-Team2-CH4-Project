#pragma once

#include "CoreMinimal.h"

// BARUGame 전용 로그 카테고리 선언
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaru, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruNet, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruGAS, Log, All);

// 호출 위치(함수명, 라인) 매크로
#define BARU_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

// 1. 단순 로깅 매크로
#define BARU_LOG(Verbosity, Format, ...) \
    UE_LOG(LogBaru, Verbosity, TEXT("[%s] %s"), *BARU_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

// 2. 멀티플레이 네트워크 전용 로깅 매크로 (NetMode + NetRole + Actor명 자동 출력)
#define BARU_NET_LOG(WorldContext, Verbosity, Format, ...) \
    do { \
        FString NetPrefix = TEXT("[UNKNOWN]"); \
        FString RolePrefix = TEXT("[NoRole]"); \
        if (IsValid(WorldContext)) { \
            if (const UWorld* World = WorldContext->GetWorld()) { \
                NetPrefix = World->IsNetMode(NM_DedicatedServer) ? TEXT("[SERVER]") : \
                            World->IsNetMode(NM_Client) ? FString::Printf(TEXT("[CLIENT_%d]"), GPlayInEditorID) : \
                            World->IsNetMode(NM_ListenServer) ? TEXT("[LISTEN_SERVER]") : TEXT("[STANDALONE]"); \
            } \
            if (const AActor* ActorContext = Cast<AActor>(WorldContext)) { \
                const ENetRole LocalRole = ActorContext->GetLocalRole(); \
                RolePrefix = (LocalRole == ROLE_Authority) ? TEXT("[Authority]") : \
                             (LocalRole == ROLE_AutonomousProxy) ? TEXT("[Autonomous]") : \
                             (LocalRole == ROLE_SimulatedProxy) ? TEXT("[Simulated]") : TEXT("[None]"); \
            } \
        } \
        UE_LOG(LogBaruNet, Verbosity, TEXT("%s%s[%s] %s"), *NetPrefix, *RolePrefix, *BARU_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__)); \
    } while (0)

// 3. 화면 디버그 메시지 출력 매크로 (화면 + 콘솔 동시 출력)
#define BARU_SCREEN_LOG(Color, Format, ...) \
    do { \
        FString Msg = FString::Printf(Format, ##__VA_ARGS__); \
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, Color, Msg); \
        UE_LOG(LogBaru, Log, TEXT("[%s] %s"), *BARU_CALLINFO, *Msg); \
    } while (0)