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
#include "Interfaces/InteractableInterface.h"   //[추가] 상호작용 대상 호출용
#include "GameplayTags/BaruGameplayTags.h"                        
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
}

// [추가] bIsDead 복제 등록. 없으면 클라이언트에서 시체 연출이 안 나옴
void ABaruCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ABaruCharacter, bIsDead);
}

void ABaruCharacter::PossessedBy(AController* NewController)
{
   Super::PossessedBy(NewController);

   InitAbilityActorInfo();

   // [추가] 리스폰 대응.
   //   ABaruTestGameMode::RespawnPlayer 는 SetDBNOState(false) 만 호출하고
   //   사망 상태는 초기화해주지 않습니다(제 권한 밖 파일).
   //   "새 폰에 빙의됐다 = 살아있는 몸을 새로 받았다" 이므로 여기서 풀어줍니다.
   //   ※ Super::PossessedBy 안에서 SetPlayerState() 가 먼저 수행되므로 이 시점엔 PS가 유효합니다.
   if (HasAuthority())
   {
      if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
      {
         BaruPS->SetDeadState(false);
      }
   }
}

// [클라이언트 전용] 서버로부터 PlayerState가 복제되어 로컬에 도착했을 때 호출됨
void ABaruCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    // 클라이언트 측 GAS ActorInfo 초기화
    InitAbilityActorInfo();
}

// GAS 초기화 공통 헬퍼 함수 (멀티플레이 대응)
void ABaruCharacter::InitAbilityActorInfo()
{
   // [수정] 중첩 if 를 early return 으로 펴고, 아래 "1회성 초기화" 구간을 분리
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

   // 캐릭터와 PlayerState를 GAS에 연결 (여러 번 호출돼도 안전)
   ASC->InitAbilityActorInfo(BaruPS, this);

   if (UBaruHealthComponent* PSHealthComp = BaruPS->GetHealthComponent())
   {
      // PlayerState 쪽 PostInitializeComponents 에서도 초기화하도록 바꿨지만,
      //  InitializeWithAbilitySystem은 같은 ASC면 즉시 return 하므로 중복 호출이 안전
      PSHealthComp->InitializeWithAbilitySystem(ASC);
   }

   // [추가] --- 여기서부터는 반드시 "한 번만" 실행되어야 하는 구간 ---
   if (CachedASC.Get() == ASC)
   {
      return;
   }
   CachedASC = ASC;

   // [추가] MoveSpeed 어트리뷰트 → CharacterMovement 연결.
   //   CoreAttributeSet 에 MoveSpeed(450)가 정의되고 복제까지 되는데
   //   MaxWalkSpeed 로 꽂아주는 코드가 프로젝트 전체에 한 줄도 없었음
   //   (= 버프/디버프로 이동속도를 바꿔도 실제로는 아무 일도 안 일어남)
   MoveSpeedChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
       UBaruCoreAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ABaruCharacter::HandleMoveSpeedChanged);

   // [08.30] 속도가 0.0f로 덮어써져 멈추는 현상을 방어하기 위한 코드.
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

// [추가] 어트리뷰트가 서버에서 바뀌면 각 클라에도 복제되어 동일하게 호출됨(RPC 불필요)
void ABaruCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
   if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
   {
      if (ChangeData.NewValue > 0.0f)
      {
         MoveComp->MaxWalkSpeed = ChangeData.NewValue;
      }
   }
}


// [추가] 델리게이트 정리. 없으면 폰 파괴 후 죽은 포인터로 콜백이 갈 수 있음
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

   // [추가] 생성자 값은 C++ 기본값일 뿐이고 BP에서 CameraEyeHeight 를 바꾼 경우는
   // 생성자 이후에 반영되므로 여기서 다시 적용합니다.
   if (FollowCamera)
   {
      FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, CameraEyeHeight));
   }
}

void ABaruCharacter::PawnClientRestart()
{
   Super::PawnClientRestart();

   // 로컬 클라이언트 컨트롤러 방어 코드
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
   }
}

void ABaruCharacter::Move(const FInputActionValue& Value)
{
   if (bIsDead)   // [추가] 사망 후 조작 차단
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

   // ★ GameMode 통지는 "다음 틱"으로 미룸
   //   GAS 콜백 도중에 폰을 떼거나 파괴하는 건 위험하므로 한 틱 미룸
   if (UWorld* World = GetWorld())
   {
      World->GetTimerManager().SetTimerForNextTick(this, &ABaruCharacter::NotifyGameModeOfDeath);
   }
}

// [추가] 두 게임모드를 반드시 분기해야함
//   ABaruGameMode 와 ABaruTestGameMode 는 상속 관계가 전혀 없고(둘 다 AGameModeBase 직속),
//   OnPlayerDied() 는 이름만 같은 별개의 함수입니다.
//   한쪽으로만 Cast 하면 다른 맵에서는 사망 처리가 통째로 무시됨
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

// [추가] 클라이언트에서 사망이 복제되어 도착했을 때의 연출 진입
//   Multicast RPC 대신 복제 변수 + OnRep 을 쓰는 이유:
//   나중에 접속한 클라이언트도 올바른 상태를 받고, RPC 트래픽이 늘지 않기 때문입
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

// ==============================================================================
// 트레이스 / RPC
// ==============================================================================

// [수정] 트레이스 채널과 디버그 표시 방식 변경
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