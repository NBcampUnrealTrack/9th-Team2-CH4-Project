// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Weapon/BaruWeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h" // UGameplayEffect
#include "BaruLog.h"        // BARU_NET_LOG
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
	// GAS 인터페이스와 ASC 타입을 사용하기 위함
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h" // PlayerState를 Cast할 때 전체 클래스 정의 필요(안 하면 cast.h 오류 발생)
	// 무기 비활성 시 위치.
#include "Components/PrimitiveComponent.h"

ABaruWeaponBase::ABaruWeaponBase()
{
		// 무기는 매 프레임 Tick할 필요 없음
	PrimaryActorTick.bCanEverTick = false;

		// 다른 플레이어에게도 장착 무기가 보이도록 Actor 복제
	bReplicates = true;

	// 무기는 Character에 붙어서 움직이므로 별도 위치 복제는 하지 않음
	SetReplicateMovement(false);

	// 무기의 보이는 몸체이자 RootComponent
	WeaponMesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("WeaponMesh"));

	SetRootComponent(WeaponMesh);

	// 장착 무기는 충돌 판정을 하지 않음
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

	// 무기를 다른 사람한테도 보이도록.
void ABaruWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// 무기 Actor를 소유한 플레이어에게는 숨기고,
	// 다른 플레이어의 화면에는 표시합니다.
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		PrimitiveComponent->SetOwnerNoSee(true);
		PrimitiveComponent->SetOnlyOwnerSee(false);
	}
}


	// 서버가 검증된 무기로 실제 발사 판정을 수행.
void ABaruWeaponBase::Fire(AActor* WeaponInstigator)
{
		// 서버 외부나 비정상적인 요청은 거절.
    if (!HasAuthority() || !IsValid(WeaponInstigator))
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World) || Range <= 0.0f || Damage <= 0.0f)
    {
        return;
    }

		// 서버 시간 기준 발사 간격 검사.
    const double CurrentTime = World->GetTimeSeconds();

    if (CurrentTime < NextAllowedFireTime)
    {
        return;
    }

    NextAllowedFireTime =
        CurrentTime + FMath::Max(static_cast<double>(FireInterval), 0.01);

		// 발사한 Character의 눈 위치를 시작점으로 사용.
		// 발사 시작 위치를 바꿀 시 여기서 조정. 근데 게임은 눈(카메라)에서 쏴야 그 느낌이 나지 않나?
    FVector TraceStart;
    FRotator ViewRotation;
    WeaponInstigator->GetActorEyesViewPoint(TraceStart, ViewRotation);

		// 플레이어가 보는 방향은 Controller의 ControlRotation을 우선 사용.
    if (const APawn* InstigatorPawn = Cast<APawn>(WeaponInstigator))
    {
        if (const AController* Controller = InstigatorPawn->GetController())
        {
            ViewRotation = Controller->GetControlRotation();
        }
    }

    const FVector TraceEnd =
        TraceStart + ViewRotation.Vector() * Range;

    FCollisionQueryParams TraceParams(
        SCENE_QUERY_STAT(BaruWeaponFire),
        false);

		// 자기 자신과 손에 든 무기는 명중 대상에서 제외. 안 하면 눈에서 쏘는 라인트레이스 빔이 막힐 수 있음.
    TraceParams.AddIgnoredActor(WeaponInstigator);
    TraceParams.AddIgnoredActor(this);

    FHitResult HitResult;

		// CombatHit = ECC_GameTraceChannel2
		// 서버가 직접 명중 여부를 확정.
    const bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_GameTraceChannel2,
        TraceParams);

    if (!bHit || !IsValid(HitResult.GetActor()))
    {
        BARU_NET_LOG(
            WeaponInstigator,
            LogBaruCombat,
            Verbose,
            TEXT("Weapon Fire Miss"));

        return;
    }

    BARU_NET_LOG(
        WeaponInstigator,
        LogBaruCombat,
        Log,
        TEXT("Weapon Fire Hit: %s"),
        *GetNameSafe(HitResult.GetActor()));

    // 다음 단계:
    // HitResult.GetActor()의 ASC를 찾고
    // DamageEffectClass를 ApplyDamageEffectToTarget()으로 적용.
		//AbilitySystem.h 참고.
	// 이 무기에 Damage GameplayEffect가 지정되지 않았다면 피해를 적용하지 않음.
	if (!DamageEffectClass)
	{
		BARU_NET_LOG(WeaponInstigator, LogBaruCombat, Warning,
			TEXT("Weapon Fire 실패: DamageEffectClass가 비어 있습니다."));
		return;
	}

		// 플레이어 Character의 ASC는 PlayerState에 있으므로,
		// 발사자를 바로 찾지 않고 발사자의 PlayerState에서 ASC를 찾음.
	const APawn* InstigatorPawn = Cast<APawn>(WeaponInstigator);
	const IAbilitySystemInterface* SourceASI =
		InstigatorPawn
		? Cast<IAbilitySystemInterface>(InstigatorPawn->GetPlayerState())
		: Cast<IAbilitySystemInterface>(WeaponInstigator);

	UBaruAbilitySystemComponent* SourceASC =
		SourceASI
		? Cast<UBaruAbilitySystemComponent>(SourceASI->GetAbilitySystemComponent())
		: nullptr;

		// 명중한 몬스터/캐릭터가 가진 ASC를 찾음.
	IAbilitySystemInterface* TargetASI =
		Cast<IAbilitySystemInterface>(HitResult.GetActor());

	UAbilitySystemComponent* TargetASC =
		TargetASI
		? TargetASI->GetAbilitySystemComponent()
		: nullptr;

	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		BARU_NET_LOG(WeaponInstigator, LogBaruCombat, Warning,
			TEXT("Weapon Fire 실패: 공격자 또는 피격자의 ASC를 찾지 못했습니다. Target=%s"),
			*GetNameSafe(HitResult.GetActor()));
		return;
	}

		// 기존 GE_Damage_Execution을 적용하고,
		// 무기 DataAsset에서 가져온 Damage 값을 물리 피해로 전달.
	SourceASC->ApplyDamageEffectToTarget(
		DamageEffectClass,
		TargetASC,
		Damage);
}

	// 서버가 DataAsset의 정적 설정값을,
	// 실제로 생성된 무기 Actor의 런타임 값으로 복사합니다.
void ABaruWeaponBase::InitializeFromData(
	const UBaruWeaponDataAsset* WeaponData)
{
		// 무기 수치는 서버가 결정.
	if (!HasAuthority() || !WeaponData)
	{
		return;
	}

	Damage = WeaponData->BaseDamage;
	Range = WeaponData->MaxRange;
	MagazineCapacity = WeaponData->MagazineCapacity;
	FireInterval = WeaponData->FireInterval;
	
	//----- 비활성 무기 부착
		// 무기별 비활성 보관 위치 정보도 런타임 Weapon Actor에 복사.
	HolsterSocketName = WeaponData->HolsterSocketName;
	HolsterRelativeTransform = WeaponData->HolsterRelativeTransform;
	HandRelativeTransform = WeaponData->HandRelativeTransform;
	
		// Soft Class는 발사할 때마다 에셋을 로드하지 않기 위해, 장착할 때 한 번만 실제 클래스로 불러옴.
	DamageEffectClass = WeaponData->DamageEffectClass.LoadSynchronous();

	if (!DamageEffectClass)
	{
		BARU_NET_LOG(
			this,
			LogBaruItem,
			Warning,
			TEXT("무기 초기화 경고: DamageEffectClass가 지정되지 않았습니다."));
	}
	

		// 변경된 복제 값을 다음 네트워크 갱신 때 전달하도록 요청
	ForceNetUpdate();
}

	// 생명주기.
void ABaruWeaponBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaruWeaponBase, Damage);
	DOREPLIFETIME(ABaruWeaponBase, Range);
	DOREPLIFETIME(ABaruWeaponBase, MagazineCapacity);
	DOREPLIFETIME(ABaruWeaponBase, FireInterval);
}