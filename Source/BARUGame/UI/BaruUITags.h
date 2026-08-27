// BaruUITags.h

#pragma once

#include "NativeGameplayTags.h"

namespace BaruUITags
{
	// 항상 표시되는 Main HUD 레이어
	BARUGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	
	// 인벤토리, ESC 메뉴 같은 일반 메뉴 레이어
	BARUGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	
	// 확인창, 경고창처럼 가장 위에 표시되는 레이어
	BARUGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
}