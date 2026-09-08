#include "Character/BaruCharacter.h" 
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h" 
#include "Components/CapsuleComponent.h"                          
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"                                    
#include "TimerManager.h"                                       
#include "Player/BaruPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"                                  
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"        
#include "Interfaces/InteractableInterface.h"   
#include "GameplayTags/BaruGameplayTags.h"    
#include "Gameplay/Equipment/BaruEquipmentComponent.h"   // ★[추가] 무기 장착
#include "Core/BaruGameMode.h"                                    
#include "Core/BaruTestGameMode.h"                                
#include "Components/BaruHealthComponent.h"
#include "DrawDebugHelpers.h"
#include "BaruLog.h"

ABaruCharacter::ABaruCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
   bReplicates = true;
   SetReplicateMovement(true);
   
    // 1인칭 카메라 설정 
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(RootComponent);
    FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, CameraEyeHeight)); // [수정] 60.0f 하드코딩제거
    FollowCamera->bUsePawnControlRotation = true; // 마우스 회전에 따라 카메라 회전

    //  생성 및 카메라 하위로 배치
    Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
    Mesh1P->SetupAttachment(FollowCamera);
    Mesh1P->SetOnlyOwnerSee(true);          // 나에게만 보임
    Mesh1P->SetCastShadow(false);           // 팔 메쉬 그림자 비활성화
    Mesh1P->bCastDynamicShadow = false;

    // [추가] 3인칭 기본 메쉬 설정 (내 눈에는 안 보이지만 그림자는 생성)
    GetMesh()->SetOwnerNoSee(true);         // 머리 내부 클리핑/점프 버그 방지
    GetMesh()->bCastHiddenShadow = true;    // 바닥에 내 몸통 그림자 형성

    // 1인칭 캐릭터 회전 제어 
    bUseControllerRotationYaw = true; // 마우스 좌우 회전 시 캐릭터 몸통도 함께 회전
    GetCharacterMovement()->bOrientRotationToMovement = false; 
   
   // [추가] 무기 장착 컴포넌트 생성
   EquipmentComponent = CreateDefaultSubobject<UBaruEquipmentComponent>(TEXT("EquipmentComponent"));
   
   // [추가] 크라우치 활성화.
   //   언리얼은 기본적으로 크라우치가 꺼져 있어서, 이게 없으면 Crouch() 를 불러와도 무시됨
   GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
   GetCharacterMovement()->SetCrouchedHalfHeight(60.0f);
}

// [추가] bIsDead 복제 등록. 없으면 클라이언트에서 시체 연출이 안 나옴
void ABaruCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ABaruCharacter, bIsDead);
   DOREPLIFETIME(ABaruCharacter, bIsSprinting);   // [추가]
}

void ABaruCharacter::PossessedBy(AController* NewController)
{
   Super::PossessedBy(NewController);

   InitAbilityActorInfo();
   
   if (HasAuthority())
   {
      if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
      {
         BaruPS->SetDeadState(false);
      }
   }
}


void ABaruCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitAbilityActorInfo();
}


void ABaruCharacter::InitAbilityActorInfo()
{
   ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>();
   if (!BaruPS)
   {
      return;
   }

   UAbilitySystemComponent* ASC = BaruPS->GetAbilitySystemComponent();
   if (!ASC)
   {
      return;
   }
   
   ASC->InitAbilityActorInfo(BaruPS, this);

   if (UBaruHealthComponent* PSHealthComp = BaruPS->GetHealthComponent())
   {
      // ★[수정] 초기 속도도 같은 경로로 적용
      BaseWalkSpeed = ASC->GetNumericAttribute(UBaruCoreAttributeSet::GetMoveSpeedAttribute());
      UpdateMaxWalkSpeed();
   }

   
   if (CachedASC.Get() == ASC)
   {
      return;
   }
   CachedASC = ASC;
   
   MoveSpeedChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
       UBaruCoreAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ABaruCharacter::HandleMoveSpeedChanged);

  
   const float InitialMoveSpeed = ASC->GetNumericAttribute(UBaruCoreAttributeSet::GetMoveSpeedAttribute());
   if (InitialMoveSpeed > 0.0f)
   {
      GetCharacterMovement()->MaxWalkSpeed = InitialMoveSpeed;
   }
   else
   {
      GetCharacterMovement()->MaxWalkSpeed = 450.0f;
   } 
   
   // GetCharacterMovement()->MaxWalkSpeed =
   //     ASC->GetNumericAttribute(UBaruCoreAttributeSet::GetMoveSpeedAttribute());
}

void ABaruCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
   // [수정] 어트리뷰트 값을 기준 속도로 저장하고, 스프린트/크라우치를 반영해 최종 적용
   BaseWalkSpeed = ChangeData.NewValue;
   UpdateMaxWalkSpeed();
}



void ABaruCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   if (UAbilitySystemComponent* ASC = CachedASC.Get())
   {
      ASC->GetGameplayAttributeValueChangeDelegate(
          UBaruCoreAttributeSet::GetMoveSpeedAttribute()).Remove(MoveSpeedChangedHandle);
   }
   CachedASC.Reset();

   Super::EndPlay(EndPlayReason);
}

void ABaruCharacter::BeginPlay()
{
   Super::BeginPlay();


   if (FollowCamera)
   {
      FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, CameraEyeHeight));
   }
   UpdateMaxWalkSpeed();
}

void ABaruCharacter::PawnClientRestart()
{
   Super::PawnClientRestart();

   
   APlayerController* PC = Cast<APlayerController>(GetController());
   if (!PC && GetWorld())
   {
      PC = GetWorld()->GetFirstPlayerController();
   }

   if (PC && PC->IsLocalController())
   {
      // 1. 카메라 시점을 내 캐릭터로 확실하게 전환
      PC->SetViewTarget(this);

      // 2. 1인칭 게임 입력 모드 설정 (마우스 커서 숨김)
      FInputModeGameOnly InputModeData;
      InputModeData.SetConsumeCaptureMouseDown(false);
      PC->SetInputMode(InputModeData);
      PC->SetShowMouseCursor(false);

      // 3. IMC 등록
      if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
      {
         if (DefaultMappingContext)
         {
            Subsystem->ClearAllMappings();
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
            BARU_LOG(LogBaru, Log, TEXT("PawnClientRestart: [SUCCESS] ViewTarget & IMC applied for %s"), *GetName());
         }
      }
   }
}

void ABaruCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
   Super::SetupPlayerInputComponent(PlayerInputComponent);

   BARU_LOG(LogBaru, Log, TEXT("SetupPlayerInputComponent on %s (MoveAction=%s, LookAction=%s)"),
       *GetName(), *GetNameSafe(MoveAction), *GetNameSafe(LookAction));

   // Enhanced Input Component 바인딩
   if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
   {
      if (MoveAction)
      {
         EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABaruCharacter::Move);
      }
      if (LookAction)
      {
         EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABaruCharacter::Look);
      }
      if (JumpAction)
      {
         EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started,   this, &ABaruCharacter::Input_Jump);
         EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABaruCharacter::Input_StopJumping);
      }
      if (InteractAction)
      {
         EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_Interact);
      }
      // [추가] 발사
      if (FireAction)
      {
         EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_Fire);
      }

      // [추가] 달리기 — 누르는 동안만 (Started/Completed 쌍)
      if (SprintAction)
      {
         EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started,   this, &ABaruCharacter::Input_SprintStart);
         EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABaruCharacter::Input_SprintStop);
      }

      // [추가] 앉기 — 누를 때마다 토글
      if (CrouchAction)
      {
         EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_ToggleCrouch);
      }
   }
}

void ABaruCharacter::Move(const FInputActionValue& Value)
{
   if (bIsDead)   // 사망 후 조작 차단
   {
      return;
   }
   FVector2D MovementVector = Value.Get<FVector2D>();
   
   if (Controller != nullptr)
   {
      // 컨트롤러 시점(Yaw) 기준으로 방향 계산
      const FRotator Rotation = Controller->GetControlRotation();
      const FRotator YawRotation(0, Rotation.Yaw, 0);

      const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
      const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

      AddMovementInput(ForwardDirection, MovementVector.Y);
      AddMovementInput(RightDirection, MovementVector.X);
   }
}

void ABaruCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
   
    if (Controller != nullptr)
    {
       AddControllerYawInput(LookAxisVector.X);
       AddControllerPitchInput(LookAxisVector.Y);
    }
}

// [추가]
void ABaruCharacter::Input_Jump()
{
   if (bIsDead)
   {
      return;
   }
   Jump();
}

// [추가]
void ABaruCharacter::Input_StopJumping()
{
   StopJumping();
}

// ★[추가] 클라이언트는 예측/연출용으로 트레이스하고, 실제 판정은 서버에 요청
void ABaruCharacter::Input_Interact()
{
   if (bIsDead)
   {
      return;
   }

   FHitResult HitResult;
   if (!PerformLineTrace(HitResult, InteractionTraceDistance, /*bDrawDebug=*/true))
   {
      return;
   }

   if (!IsValid(HitResult.GetActor()))
   {
      return;
   }

   Server_ProcessInteraction(HitResult);
}

// ★[추가] 발사 요청.
//   캐릭터는 "쐈다"는 사실만 전달합니다.
//   호스트/클라 분기, 서버 라인트레이스, GE 적용은 전부 EquipmentComponent 와 WeaponBase 담당입니다.
void ABaruCharacter::Input_Fire()
{
   if (bIsDead || !EquipmentComponent)
   {
      return;
   }

   EquipmentComponent->RequestFireActiveWeapon();
}

// ★[추가] 달리기 시작/종료.
//   속도는 서버가 확정해야 위치 보정이 안 생기므로 RPC 로 요청만 합니다.
void ABaruCharacter::Input_SprintStart()
{
   if (bIsDead)
   {
      return;
   }
   Server_SetSprinting(true);
}

void ABaruCharacter::Input_SprintStop()
{
   Server_SetSprinting(false);
}

bool ABaruCharacter::Server_SetSprinting_Validate(bool bNewSprinting)
{
   return true;   // bool 하나뿐이라 조작 가능한 값이 없습니다.
}

void ABaruCharacter::Server_SetSprinting_Implementation(bool bNewSprinting)
{
   if (bIsSprinting == bNewSprinting)
   {
      return;
   }

   bIsSprinting = bNewSprinting;
   OnRep_IsSprinting();   // 서버는 OnRep 이 자동 호출되지 않으므로 직접 호출
}

// ★[추가] 스프린트 상태가 복제되어 도착했을 때 각 클라에서 속도 갱신
void ABaruCharacter::OnRep_IsSprinting()
{
   UpdateMaxWalkSpeed();
}

// ★[추가] 앉기 토글.
//   Crouch() / UnCrouch() 는 언리얼이 서버·클라 예측까지 알아서 처리하므로
//   별도 RPC 가 필요 없습니다. (생성자의 bCanCrouch = true 가 전제)
void ABaruCharacter::Input_ToggleCrouch()
{
   if (bIsDead)
   {
      return;
   }

   if (bIsCrouched)
   {
      UnCrouch();
   }
   else
   {
      Crouch();
   }
}

// ★[추가] 현재 상태(앉기/달리기)에 맞는 이동 속도를 계산해 적용합니다.
//   서버와 클라 양쪽에서 같은 계산을 하므로 값이 어긋나지 않습니다.
void ABaruCharacter::UpdateMaxWalkSpeed()
{
   UCharacterMovementComponent* MoveComp = GetCharacterMovement();
   if (!MoveComp)
   {
      return;
   }

   MoveComp->MaxWalkSpeedCrouched = CrouchedWalkSpeed;
   MoveComp->MaxWalkSpeed = bIsSprinting ? (BaseWalkSpeed * SprintSpeedMultiplier) : BaseWalkSpeed;
}

void ABaruCharacter::OnRep_Controller()
{
   Super::OnRep_Controller();
    
   // 클라이언트 측에 컨트롤러가 동기화된 시점에 GAS ActorInfo 갱신
   InitAbilityActorInfo();
}

// ==============================================================================
// CombatInterface 함수 구현부 
// ==============================================================================

// [수정] 전면 재작성. 기존엔 로그 한 줄만 찍고 끝나서
//   GameMode 의 생존자 집계/전멸 판정/관전 전환이 전부 돌지 않았음
void ABaruCharacter::Die_Implementation(AActor* Killer)
{
   if (bIsDead)
   {
      return;   // 중복 사망 방지
   }

   if (!HasAuthority())
   {
      return;   // 사망 확정은 서버만. 클라는 bIsDead 복제를 받아 연출만 재생.
   }

   bIsDead = true;
   LastKiller = Killer;

   // 서버에서는 OnRep 이 자동 호출되지 않으므로 직접 호출(리슨서버 호스트 화면 연출용)
   OnRep_IsDead();

   // 서버 전용 물리/이동 정리
   if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
   {
      MoveComp->StopMovementImmediately();
      MoveComp->DisableMovement();
   }
   if (UCapsuleComponent* Capsule = GetCapsuleComponent())
   {
      Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   }

   if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
   {
      ASC->CancelAllAbilities();
   }

   // PlayerState 에 사망 상태 기록 (GameMode 의 생존자 집계가 PS를 봅니다)
   if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      BaruPS->SetDeadState(true, Killer);
   }

   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Character %s has died. Killer: %s"),
       *GetName(), Killer ? *Killer->GetName() : TEXT("None"));

  
   if (UWorld* World = GetWorld())
   {
      World->GetTimerManager().SetTimerForNextTick(this, &ABaruCharacter::NotifyGameModeOfDeath);
   }
}

void ABaruCharacter::NotifyGameModeOfDeath()
{
   if (!HasAuthority() || !GetWorld())
   {
      return;
   }

   AGameModeBase* AuthGM = GetWorld()->GetAuthGameMode();
   if (!AuthGM)
   {
      return;
   }

   if (ABaruGameMode* BaruGM = Cast<ABaruGameMode>(AuthGM))
   {
      BaruGM->OnPlayerDied(GetController(), LastKiller);
      return;
   }

   if (ABaruTestGameMode* TestGM = Cast<ABaruTestGameMode>(AuthGM))
   {
      TestGM->OnPlayerDied(GetController(), LastKiller);
      return;
   }

   BARU_NET_LOG(this, LogBaruCombat, Warning,
       TEXT("Death not reported: unknown GameMode class %s"), *AuthGM->GetClass()->GetName());
}

void ABaruCharacter::OnRep_IsDead()
{
   if (!bIsDead)
   {
      return;   // 리스폰으로 false 가 복제된 경우
   }

   if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
   {
      MoveComp->StopMovementImmediately();
   }
   if (UCapsuleComponent* Capsule = GetCapsuleComponent())
   {
      Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   }

   // State.Dead 태그를 각자 로컬로 부여 (BaruGameplayTags 에 이미 정의되어 있음)
   if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
   {
      ASC->AddLooseGameplayTag(FBaruGameplayTags::Get().State_Dead);
   }

   // 죽으면 1인칭 팔을 숨기고 3인칭 몸(시체)을 본인에게도 보이게 전환
   if (Mesh1P)
   {
      Mesh1P->SetHiddenInGame(true);
   }
   if (USkeletalMeshComponent* BodyMesh = GetMesh())
   {
      BodyMesh->SetOwnerNoSee(false);
   }

   OnDeathCosmetic();   // 래그돌 / 사망 몽타주는 BP 에서
}

// [수정] 체력에서 유도하지 않고 상태 변수를 그대로 반환.
//   이 한 줄이 캐릭터가 죽지 않는버그의 핵심 수정
bool ABaruCharacter::IsDead_Implementation() const
{
   return bIsDead;
}

// [추가] 이하 7개는 미구현이라 UHT 기본 스텁(0 / false)이 반환되고 있던 함수들
bool ABaruCharacter::IsDBNO_Implementation() const
{
   if (const ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      return BaruPS->IsDBNO();
   }
   return false;
}

bool ABaruCharacter::IsGroggy_Implementation() const
{
   // 그로기제압은 현재 몬스터 전용 개념(BaruMonsterAttributeSet)이라 플레이어는 항상 false.
   // Todo: 플레이어 그로기 사양이 정해지면 State 태그로 판정하도록 교체
   return false;
}

float ABaruCharacter::GetCurrentHealth_Implementation() const
{
   if (const ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      return BaruPS->GetHealth();
   }
   return 0.0f;
}

float ABaruCharacter::GetMaxHealth_Implementation() const
{
   if (const ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      return BaruPS->GetMaxHealth();
   }
   return 0.0f;
}

float ABaruCharacter::GetSuppressionRatio_Implementation() const
{
   // 제압도(Suppression)는 BaruMonsterAttributeSet 에만 존재. 플레이어는 0 고정.
   return 0.0f;
}

void ABaruCharacter::ApplyCombatDamage_Implementation(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser, AController* InstigatedBy)
{
   // 여기서 체력을 직접 깎으면 GAS 를 우회하게 되어 서버/클라 값이 어긋남
   // 데미지는 반드시 GameplayEffect(BaruDamageExecutionCalc)로만 적용하도록 해야함
   // 이 함수는 피격 리액션 몽타주 / 히트 사운드 같은 연출 훅으로만 사용함
}

void ABaruCharacter::BreakBodyPart_Implementation(FName BoneName, float Damage)
{
   // 부위 파괴는 몬스터 전용 사양. 플레이어는 미사용.
}

// 트레이스 / RPC
bool ABaruCharacter::PerformLineTrace(FHitResult& OutHitResult, float TraceDistance, bool bDrawDebug)
{
   AController* PC = GetController();
   if (!PC)
   {
      return false;
   }

   UWorld* World = GetWorld();
   if (!World)
   {
      return false;
   }

   // 카메라 위치 및 바라보는 방향 가져오기
   FVector CameraLocation;
   FRotator CameraRotation;
   PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

   const FVector TraceStart = CameraLocation;
   const FVector TraceEnd = TraceStart + (CameraRotation.Vector() * TraceDistance);

   FCollisionQueryParams TraceParams(FName(TEXT("BaruLineTrace")), true, this);
   TraceParams.bReturnPhysicalMaterial = true;
   TraceParams.AddIgnoredActor(this);   // [추가] 명시적으로 자기 자신 제외

   // [수정] ECC_Visibility → 프로젝트 전용 채널.
   //   Visibility 로 두면 데칼/반투명 벽 같은 것에도 전부 걸림
   const bool bHit = World->LineTraceSingleByChannel(
       OutHitResult,
       TraceStart,
       TraceEnd,
       InteractionTraceChannel,
       TraceParams
   );

#if ENABLE_DRAW_DEBUG
   // [수정] WITH_EDITOR → ENABLE_DRAW_DEBUG, 그리고 호출자가 켤 때만 그림
   if (bDrawDebug)
   {
      const FColor LineColor = bHit ? FColor::Green : FColor::Red;
      DrawDebugLine(World, TraceStart, bHit ? OutHitResult.ImpactPoint : TraceEnd, LineColor, false, 2.0f, 0, 1.0f);
   }
#endif

   return bHit;
}

// [수정] 무조건 true 반환 금지.
//   _Validate 가 false 를 반환하면 커넥션이 끊기기때문에 즉 "명백히 조작된 패킷"만 여기서 걸러야 하고,
//   단순히 조건이 안 맞는 경우는 _Implementation 안에서 return 으로 처리해야함
bool ABaruCharacter::Server_ProcessInteraction_Validate(const FHitResult& HitResult)
{
   if (HitResult.ImpactPoint.ContainsNaN() || HitResult.TraceStart.ContainsNaN() || HitResult.TraceEnd.ContainsNaN())
   {
      return false;
   }
   return true;
}

void ABaruCharacter::Server_ProcessInteraction_Implementation(const FHitResult& HitResult)
{
   // [수정] 기존엔 클라이언트가 보낸 HitResult 를 아무 검증 없이 신뢰했기때문
   if (bIsDead)
   {
      return;
   }

   AActor* ClaimedActor = HitResult.GetActor();
   if (!IsValid(ClaimedActor))
   {
      return;
   }

   // 1) 거리 검증 — 맵 반대편 액터를 조작해서 보내는 것을 차단
   const float MaxDist = InteractionTraceDistance + InteractionLagTolerance;
   if (FVector::DistSquared(GetActorLocation(), HitResult.ImpactPoint) > FMath::Square(MaxDist))
   {
      BARU_NET_LOG(this, LogBaruNet, Warning, TEXT("Interaction rejected: target out of range (%s)"), *ClaimedActor->GetName());
      return;
   }

   // 2) 서버 시점 재트레이스 — 서버가 직접 확인한 대상과 일치할 때만 인정
   //    (컨트롤 로테이션은 이동 패킷으로 서버에 복제되어 있으므로 서버에서도 트레이스가 가능합니다)
   FHitResult ServerHit;
   if (!PerformLineTrace(ServerHit, MaxDist, /*bDrawDebug=*/false) || ServerHit.GetActor() != ClaimedActor)
   {
      BARU_NET_LOG(this, LogBaruNet, Warning, TEXT("Interaction rejected: server re-trace mismatch (%s)"), *ClaimedActor->GetName());
      return;
   }

   // 3) 확정 처리
   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Server Processed Interaction with: %s"), *ClaimedActor->GetName());

   if (!ClaimedActor->Implements<UInteractableInterface>())
   {
      BARU_NET_LOG(this, LogBaruItem, Verbose,
          TEXT("Interaction target does not implement InteractableInterface: %s"), *ClaimedActor->GetName());
      return;
   }

   // CanInteract 판단은 대상이 스스로 합니다(쿨다운, 이미 열린 문, 인벤토리 가득참 등).
   if (!IInteractableInterface::Execute_CanInteract(ClaimedActor, this))
   {
      BARU_NET_LOG(this, LogBaruItem, Log,
          TEXT("Interaction refused by target: %s"), *ClaimedActor->GetName());
      return;
   }

   IInteractableInterface::Execute_ExecuteInteraction(ClaimedActor, this);

   BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Interaction executed on: %s"), *ClaimedActor->GetName());
}

UAbilitySystemComponent* ABaruCharacter::GetAbilitySystemComponent() const
{
   // PlayerState에서 ASC를 가져오거나, 캐릭터가 가지고 있는 ASC를 반환
   if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      return BaruPS->GetAbilitySystemComponent();
   }
   return nullptr;
}