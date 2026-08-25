#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// 도메인별 로그 카테고리 선언
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaru, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruNet, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruGAS, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruCombat, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruAI, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruItem, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruUI, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruSession, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruSanity, Log, All);
BARUGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogBaruBackend, Log, All);

// 네트워크 상태 문자열 추출 헬퍼 (구현부는 cpp로 완전히 격리)
namespace BaruLog
{
	BARUGAME_API FString GetNetPrefix(const UObject* WorldContext);
}

// 호출 위치 문자열 매크로
#define BARU_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

// [매크로 1] 기본 로깅 (카테고리 지정)
#define BARU_LOG(Category, Verbosity, Format, ...) \
UE_LOG(Category, Verbosity, TEXT("[%s] %s"), *BARU_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

// [매크로 2] 멀티플레이 전용 로깅 (NetMode + Role + Actor명 자동 부착)
#define BARU_NET_LOG(WorldContext, Category, Verbosity, Format, ...) \
UE_LOG(Category, Verbosity, TEXT("%s[%s] %s"), *BaruLog::GetNetPrefix(WorldContext), *BARU_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

// [매크로 3] 화면 디버그 메시지 + 콘솔 출력
#define BARU_SCREEN_LOG(Color, Format, ...) \
do { \
FString Msg = FString::Printf(Format, ##__VA_ARGS__); \
if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, Color, FString::Printf(TEXT("[%s] %s"), *BARU_CALLINFO, *Msg)); \
UE_LOG(LogBaru, Log, TEXT("[%s] %s"), *BARU_CALLINFO, *Msg); \
} while (0)