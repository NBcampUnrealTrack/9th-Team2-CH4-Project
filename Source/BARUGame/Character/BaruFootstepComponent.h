// BaruFootstepComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaruFootstepComponent.generated.h"

class ACharacter;
class USoundBase;
class USoundAttenuation;
struct FHitResult;

/**
 * ★[추가 09.11] 캐릭터 발걸음 사운드 컴포넌트.
 *
 * 애니메이션 노티파이 대신 "이동 거리"로 발소리를 냅니다.
 *   - 걷기/달리기 애니메이션이 External 공유 에셋(약 60개)이라 노티파이를 박으면 팀 충돌 위험이 큽니다.
 *   - 무기별 블렌드스페이스·크라우치가 바뀌어도 코드 수정 없이 동작합니다.
 *
 * 멀티플레이: 각 클라이언트가 자기 화면에 보이는 모든 캐릭터에 대해 로컬로 재생합니다.
 *   이동·앉기·달리기 상태가 이미 복제되고 있어서 RPC 가 필요 없습니다.
 */
UCLASS(ClassGroup=(BARU), meta=(BlueprintSpawnableComponent))
class BARUGAME_API UBaruFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaruFootstepComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 발소리 한 번 재생. VolumeScale 은 상태별 볼륨 배율입니다.
	UFUNCTION(BlueprintCallable, Category = "BARU|Footstep")
	void PlayFootstep(float VolumeScale = 1.0f);

protected:
	virtual void BeginPlay() override;

	// ── 사운드 ─────────────────────────────────────
	// 발소리 후보. 여러 개 넣으면 매번 다른 소리를 골라 반복감을 줄입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	TArray<TObjectPtr<USoundBase>> DefaultFootstepSounds;

	// 거리 감쇠. 비워두면 맵 어디서든 같은 크기로 들립니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	TObjectPtr<USoundAttenuation> FootstepAttenuation;

	// ── 보폭(cm). 이 거리만큼 움직일 때마다 한 번 재생합니다. ──
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float WalkStrideLength = 170.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float SprintStrideLength = 230.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float CrouchStrideLength = 110.0f;

	// ── 상태별 볼륨 ─────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float WalkVolume = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float SprintVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float CrouchVolume = 0.35f;

	// 이 속도(cm/s) 미만이면 발소리를 내지 않습니다. 제자리 미세 이동 방지.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float MinSpeedForFootstep = 50.0f;

	// ── 착지 ───────────────────────────────────────
	// 이 시간(초) 이상 공중에 있었을 때만 착지음을 냅니다. 턱·계단에서 연발 방지.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float MinAirTimeForLandingSound = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep")
	float LandingVolumeScale = 1.25f;

	// ── 몬스터 청각용 훅 ─────────────────────────────
	// 몬스터팀이 AI Perception 의 Hearing 을 붙이면 바로 동작합니다.
	// 지금은 듣는 쪽이 없어서 호출해도 아무 일도 일어나지 않습니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep|AI")
	bool bReportAINoise = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Footstep|AI")
	float AINoiseRange = 1500.0f;

private:
	bool IsOwnerIncapacitated() const;
	void GetStateParams(float& OutStride, float& OutVolume) const;
	bool TraceFloor(FHitResult& OutHit) const;
	USoundBase* SelectFootstepSound();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	float DistanceSinceLastStep = 0.0f;
	float AirTime = 0.0f;
	bool bWasOnGround = true;          // true 로 시작해야 스폰 순간 착지음이 안 납니다
	int32 LastSoundIndex = INDEX_NONE;
};