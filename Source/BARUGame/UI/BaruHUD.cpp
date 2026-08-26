// BaruHUD.cpp

#include "UI/BaruHUD.h"

#include "BaruLog.h"
#include "GameFramework/PlayerController.h"
#include "UI/Foundation/BaruPrimaryGameLayout.h"

ABaruHUD::ABaruHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABaruHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningPlayerController =
		GetOwningPlayerController();

	if (!IsValid(OwningPlayerController) ||
		!OwningPlayerController->IsLocalController())
	{
		return;
	}

	if (!PrimaryGameLayoutClass)
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("PrimaryGameLayoutClass가 설정되지 않았습니다."));

		return;
	}

	if (IsValid(PrimaryGameLayout))
	{
		return;
	}

	PrimaryGameLayout =
		CreateWidget<UBaruPrimaryGameLayout>(
			OwningPlayerController,
			PrimaryGameLayoutClass);

	if (!IsValid(PrimaryGameLayout))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("Primary Game Layout 생성에 실패했습니다."));

		return;
	}

	PrimaryGameLayout->AddToPlayerScreen(0);

	BARU_NET_LOG(
		this,
		LogBaruUI,
		Log,
		TEXT("Primary Game Layout 생성 완료."));
}

void ABaruHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(PrimaryGameLayout))
	{
		PrimaryGameLayout->RemoveFromParent();
		PrimaryGameLayout = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
