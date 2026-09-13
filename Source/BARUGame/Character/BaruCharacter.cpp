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
#include "Gameplay/Equipment/BaruEquipmentComponent.h"   
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"   
#include "Core/BaruGameMode.h"                                    
#include "Core/BaruTestGameMode.h"                                
#include "Components/BaruHealthComponent.h"
#include "Character/BaruFootstepComponent.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"   
#include "Gameplay/Items/BaruItemInstance.h"             
#include "Gameplay/Items/DataTypes/BaruItemData.h"       
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h" 
#include "Components/SpotLightComponent.h" 
#include "DrawDebugHelpers.h"
#include "BaruLog.h"
#include "Components/BaruTensionComponent.h"
#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"

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

   // [09.13] 카메라 렌더링 후처리 오버라이드 활성화(조준 기능)
   FollowCamera->PostProcessBlendWeight = 1.0f;
   FollowCamera->PostProcessSettings.bOverride_VignetteIntensity = true;
   FollowCamera->PostProcessSettings.VignetteIntensity = DefaultVignette;
   FollowCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
   FollowCamera->PostProcessSettings.SceneFringeIntensity = DefaultFringe;
   
    Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));

   
    Mesh1P->SetupAttachment(GetMesh());
    Mesh1P->SetOnlyOwnerSee(true);          
    Mesh1P->SetCastShadow(false);           
    Mesh1P->bCastDynamicShadow = false;
   
    GetMesh()->SetOwnerNoSee(true);        
    GetMesh()->bCastHiddenShadow = true;    

    // 1인칭 캐릭터 회전 제어 
    bUseControllerRotationYaw = true; // 마우스 좌우 회전 시 캐릭터 몸통도 함께 회전
    GetCharacterMovement()->bOrientRotationToMovement = false; 
   
   EquipmentComponent = CreateDefaultSubobject<UBaruEquipmentComponent>(TEXT("EquipmentComponent"));
   FootstepComponent = CreateDefaultSubobject<UBaruFootstepComponent>(TEXT("FootstepComponent"));
   
   // 긴장도 컴포넌트 부착
   TensionComponent = CreateDefaultSubobject<UBaruTensionComponent>(TEXT("TensionComponent"));
   
   // [추가] 헤드라이트.
   //   카메라에 붙이면 시선 방향과 정확히 일치하고,
   //   다른 클라에서도 RemoteViewPitch 로 위아래 각도가 대략 맞습니다.
   Headlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Headlight"));
   Headlight->SetupAttachment(FollowCamera);
   Headlight->SetRelativeLocation(FVector(10.0f, 0.0f, 0.0f));   // 몸에 파묻히지 않게 살짝 앞으로

   Headlight->SetIntensityUnits(ELightUnits::Lumens);
   Headlight->SetIntensity(350.0f);
   Headlight->SetAttenuationRadius(700.0f);   // 7m
   Headlight->SetInnerConeAngle(15.0f);
   Headlight->SetOuterConeAngle(28.0f);
   Headlight->SetCastShadows(true);            // 프레임 떨어지면 BP 에서 끄세요

   Headlight->SetVisibility(false);            // 시작은 꺼진 상태
   
   GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
   GetCharacterMovement()->SetCrouchedHalfHeight(60.0f);
}

// [추가] bIsDead 복제 등록. 없으면 클라이언트에서 시체 연출이 안 나옴
void ABaruCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ABaruCharacter, bIsDead);
   DOREPLIFETIME(ABaruCharacter, bIsSprinting);  
   DOREPLIFETIME(ABaruCharacter, bHeadlightOn);   // [추가]
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
   if (!BaruPS->OnDBNOStatusChanged.IsAlreadyBound(this, &ABaruCharacter::HandleDBNOStatusChanged))
   {
      BaruPS->OnDBNOStatusChanged.AddDynamic(this, &ABaruCharacter::HandleDBNOStatusChanged);
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
}

void ABaruCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
   BaseWalkSpeed = ChangeData.NewValue;
   UpdateMaxWalkSpeed();
}

void ABaruCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   // [09.13] 타이머 메모리 정리
   StopHealthRegen();
   
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
   
   if (USkeletalMeshComponent* BodyMesh = GetMesh())
   {
      TArray<USceneComponent*> ChildComponents;
      BodyMesh->GetChildrenComponents(false, ChildComponents);

      for (USceneComponent* Child : ChildComponents)
      {
         if (USkeletalMeshComponent* PartMesh = Cast<USkeletalMeshComponent>(Child))
         {
            PartMesh->SetLeaderPoseComponent(BodyMesh);
         }
      }
   }
   
   UpdateHeadlightVisual();
   UpdateMaxWalkSpeed(); 
}

void ABaruCharacter::PawnClientRestart()
{
   Super::PawnClientRestart();

   if (IsLocallyControlled() && GetMesh())
   {
      GetMesh()->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
      GetMesh()->HideBoneByName(TEXT("neck_02"), EPhysBodyOp::PBO_None);
      GetMesh()->HideBoneByName(TEXT("neck_01"), EPhysBodyOp::PBO_None);
   }
   
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
         // [추가] F 를 떼면 진행 중이던 홀드 상호작용을 취소
         EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &ABaruCharacter::Input_StopInteract);
         EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Canceled,  this, &ABaruCharacter::Input_StopInteract);
      }
      // [수정] 발사 — 누를 때 시작, 뗄 때 중지 
      if (FireAction)
      {
         EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started,   this, &ABaruCharacter::Input_Fire);
         EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABaruCharacter::Input_StopFire);
         EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Canceled,  this, &ABaruCharacter::Input_StopFire);
      }
      // [09.13] 조준 액션 바인딩
      if (AimAction)
      {
         EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started,   this, &ABaruCharacter::Input_AimStart);
         EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABaruCharacter::Input_AimStop);
         EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled,  this, &ABaruCharacter::Input_AimStop);
      }

      // ★[추가 09.08] 무기 슬롯 전환
      if (SelectPrimaryWeaponAction)
      {
         EnhancedInputComponent->BindAction(SelectPrimaryWeaponAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_SelectPrimaryWeapon);
      }
      if (SelectSecondaryWeaponAction)
      {
         EnhancedInputComponent->BindAction(SelectSecondaryWeaponAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_SelectSecondaryWeapon);
      }
      if (HeadlightAction)
      {
         EnhancedInputComponent->BindAction(HeadlightAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_ToggleHeadlight);
      }
      // [추가] 달리기 
      if (SprintAction)
      {
         EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started,   this, &ABaruCharacter::Input_SprintStart);
         EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABaruCharacter::Input_SprintStop);
      }
      // [추가] 앉기
      if (CrouchAction)
      {
         EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_ToggleCrouch);
      }
      // [09.13] 재장전 액션 바인딩 (R 키)
      if (ReloadAction)
      {
         EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABaruCharacter::Input_Reload);
      }
   }
}

void ABaruCharacter::Move(const FInputActionValue& Value)
{
   if (bIsDead || Execute_IsDBNO(this)) 
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
   if (bIsDead || Execute_IsDBNO(this))
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
   if (bIsDead || Execute_IsDBNO(this))
   {
      return;
   }

   FHitResult HitResult;
   if (!PerformLineTrace(HitResult, InteractionTraceDistance, /*bDrawDebug=*/false))
   {
      return;
   }

   if (!IsValid(HitResult.GetActor()))
   {
      return;
   }

   Server_ProcessInteraction(HitResult);
}

// [추가] F 를 뗐을 때
void ABaruCharacter::Input_StopInteract()
{
   Server_StopInteraction();
}

void ABaruCharacter::Input_Fire()
{
   if (bIsDead || Execute_IsDBNO(this) || !EquipmentComponent)
   {
      return;
   }

   EquipmentComponent->RequestFireActiveWeapon();
}
void ABaruCharacter::Input_ToggleHeadlight()
{
   if (bIsDead || Execute_IsDBNO(this))
   {
      return;
   }
   Server_SetHeadlightOn(!bHeadlightOn);
}

// [09.13] 재장전 입력 처리 함수 추가
void ABaruCharacter::Input_Reload()
{
   if (bIsDead || Execute_IsDBNO(this))
   {
      return;
   }

   // 장비 컴포넌트를 통해 InputTag.Reload 트리거
   if (EquipmentComponent)
   {
      EquipmentComponent->RequestReloadActiveWeapon();
   }
}

bool ABaruCharacter::Server_SetHeadlightOn_Validate(bool bNewOn)
{
   return true;
}

void ABaruCharacter::Server_SetHeadlightOn_Implementation(bool bNewOn)
{
   if (bHeadlightOn == bNewOn)
   {
      return;
   }

   bHeadlightOn = bNewOn;

   // 서버에서는 OnRep 이 자동 호출되지 않으므로 직접 부릅니다.
   OnRep_HeadlightOn();

   BARU_NET_LOG(this, LogBaru, Log, TEXT("Headlight: %s"), bHeadlightOn ? TEXT("ON") : TEXT("OFF"));
}

// ★[추가 09.10] 복제된 상태가 클라이언트에 도착했을 때
void ABaruCharacter::OnRep_HeadlightOn()
{
   UpdateHeadlightVisual();
}

// ★[추가 09.10] 실제 라이트 켜고 끄기. 서버·클라 공통 경로.
void ABaruCharacter::UpdateHeadlightVisual()
{
   if (Headlight)
   {
      Headlight->SetVisibility(bHeadlightOn);
   }
}
// [추가] 연사 중지.
//   사망 체크를 안 하는 이유: 죽는 순간에도 반드시 발사가 멈춰야해서
void ABaruCharacter::Input_StopFire()
{
   if (EquipmentComponent)
   {
      EquipmentComponent->RequestStopFireActiveWeapon();
   }
}

void ABaruCharacter::Input_SelectPrimaryWeapon()
{
   HandleWeaponSlotInput(EBaruEquipmentSlot::PrimaryWeapon);
}

void ABaruCharacter::Input_SelectSecondaryWeapon()
{
   HandleWeaponSlotInput(EBaruEquipmentSlot::SecondaryWeapon);
}

void ABaruCharacter::HandleWeaponSlotInput(EBaruEquipmentSlot DesiredSlot)
{
   if (bIsDead || Execute_IsDBNO(this) || !EquipmentComponent)
   {
      return;
   }
   
   if (EquipmentComponent->GetActiveWeaponSlot() == DesiredSlot)
   {
      return;
   }

   Server_RequestSelectWeaponSlot(DesiredSlot);
}

// [추가 ] 서버에서 실제 슬롯 선택 수행.
bool ABaruCharacter::Server_RequestSelectWeaponSlot_Validate(EBaruEquipmentSlot DesiredSlot)
{
   return DesiredSlot == EBaruEquipmentSlot::PrimaryWeapon
       || DesiredSlot == EBaruEquipmentSlot::SecondaryWeapon;
}

void ABaruCharacter::Server_RequestSelectWeaponSlot_Implementation(EBaruEquipmentSlot DesiredSlot)
{
   if (!EquipmentComponent)
   {
      return;
   }
   
   // 1. 해당 슬롯에 이미 장착된 무기가 있다면 활성 슬롯(손 <-> 홀스터)만 전환
   if (EquipmentComponent->GetEquippedWeaponItem(DesiredSlot) != nullptr)
   {
      EquipmentComponent->RequestSetActiveWeaponSlot(DesiredSlot);
      BARU_NET_LOG(this, LogBaruItem, Log, TEXT("무기 슬롯 전환: Slot=%d"), static_cast<int32>(DesiredSlot));
      return;
   }

   // 2. 슬롯이 비어 있을 때만 인벤토리에서 검색 후 신규 장착(UseItem) 진행
   ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>();
   UBaruInventoryComponent* Inventory = BaruPS ? BaruPS->GetInventoryComponent() : nullptr;
   if (!Inventory)
   {
      return;
   }
   
   UBaruItemInstance* TargetItem = FindInventoryWeaponForSlot(DesiredSlot);
   if (!TargetItem)
   {
      BARU_NET_LOG(this, LogBaruItem, Log,
          TEXT("슬롯 %d 에 맞는 무기가 인벤토리에 없습니다."), static_cast<int32>(DesiredSlot));
      return;
   }

   // UseItem() 이 아이템 타입을 보고 EquipWeapon() 까지 이어줍니다.
   // 장착 후 즉시 해당 무기를 손에 쥐도록 활성 슬롯으로 전환
   Inventory->UseItem(TargetItem);
   EquipmentComponent->RequestSetActiveWeaponSlot(DesiredSlot);
}

// [추가] 무기 해제
bool ABaruCharacter::Server_RequestUnequipWeapon_Validate(EBaruEquipmentSlot WeaponSlot)
{
   return WeaponSlot == EBaruEquipmentSlot::PrimaryWeapon
       || WeaponSlot == EBaruEquipmentSlot::SecondaryWeapon;
}

void ABaruCharacter::Server_RequestUnequipWeapon_Implementation(EBaruEquipmentSlot WeaponSlot)
{
   if (!EquipmentComponent)
   {
      return;
   }

   EquipmentComponent->UnequipWeapon(WeaponSlot);
   BARU_NET_LOG(this, LogBaruItem, Log, TEXT("무기 해제: Slot=%d"), static_cast<int32>(WeaponSlot));
}

UBaruItemInstance* ABaruCharacter::FindInventoryWeaponForSlot(EBaruEquipmentSlot DesiredSlot) const
{
   const ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>();
   if (!BaruPS)
   {
      return nullptr;
   }

   UBaruInventoryComponent* Inventory = BaruPS->GetInventoryComponent();
   if (!Inventory || !Inventory->ItemDataTable)
   {
      return nullptr;
   }

   for (const FInventorySlot& Slot : Inventory->GetSlots())
   {
      if (!IsValid(Slot.Item))
      {
         continue;
      }

      const FItemData* Data =
         Inventory->ItemDataTable->FindRow<FItemData>(Slot.Item->ItemID, TEXT("FindInventoryWeaponForSlot"));

      if (!Data || Data->ItemType != EItemType::Weapon)
      {
         continue;
      }

      const UBaruWeaponDataAsset* WeaponData = Data->WeaponDataAsset.LoadSynchronous();
      if (WeaponData && WeaponData->EquipmentSlot == DesiredSlot)
      {
         return Slot.Item;
      }
   }

   return nullptr;
}

void ABaruCharacter::Input_SprintStart()
{
   if (bIsDead || Execute_IsDBNO(this))
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


void ABaruCharacter::OnRep_IsSprinting()
{
   UpdateMaxWalkSpeed();
}


void ABaruCharacter::Input_ToggleCrouch()
{
   if (bIsDead || Execute_IsDBNO(this))
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

// [추가] 소생 대상이 될 수 있는지. 클라이언트에서 프롬프트 표시에도 쓰입니다.
bool ABaruCharacter::CanInteract_Implementation(APawn* Interactor) const
{
   // 나는 다운 상태여야 합니다.
   if (bIsDead || !Execute_IsDBNO(this))
   {
      return false;
   }

   // 상대는 살아있는 다른 플레이어여야 합니다.
   const ABaruCharacter* Rescuer = Cast<ABaruCharacter>(Interactor);
   if (!Rescuer || Rescuer == this)
   {
      return false;
   }

   return !Rescuer->bIsDead && !Execute_IsDBNO(Rescuer);
}

FText ABaruCharacter::GetInteractPromptText_Implementation(APawn* Interactor) const
{
   // 실제 키 표시는 나중에 UI 가 IMC 에서 읽어오도록 개선할 수 있습니다.
   return FText::FromString(TEXT("F: 소생"));
}

FGameplayTag ABaruCharacter::GetInteractionTag_Implementation() const
{
   // 전용 Revive 태그가 없어서 협동(CoOp) 태그를 씁니다.
   // 프레임워크팀에 Interaction.Type.Revive 추가를 요청해두면 바꾸면 됩니다.
   return FBaruGameplayTags::Get().Interaction_Type_CoOp;
}

float ABaruCharacter::GetInteractionDuration_Implementation() const
{
   // 0 보다 크므로 Server_ProcessInteraction 이 홀드 방식으로 처리합니다.
   // 중간에 F 를 떼거나 멀어지면 자동 취소됩니다.
   return ReviveDuration;
}

void ABaruCharacter::ExecuteInteraction_Implementation(APawn* Interactor)
{
   // 서버에서만 호출됩니다(Server_ProcessInteraction 경유).
   ReviveFromDBNO(ReviveHealthRatio);

   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("%s revived by %s"),
       *GetName(), Interactor ? *Interactor->GetName() : TEXT("None"));
}

// [추가] DBNO 진입. 서버 전용.
//   BaruCoreAttributeSet::PostGameplayEffectExecute 가 첫 체력 0 도달 시 호출합니다.
void ABaruCharacter::EnterDBNO(AActor* DownCauser)
{
   if (!HasAuthority() || bIsDead)
   {
      return;
   }
   
   // [09.13] 다운 시 회복 즉시 중단
   StopHealthRegen();

   ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>();
   if (!BaruPS || BaruPS->IsDBNO())
   {
      return;   // 이미 다운 상태
   }

   LastKiller = DownCauser;   // 블리드아웃으로 죽으면 이 사람이 킬 크레딧

   // 진행 중이던 상호작용·발사를 정리합니다.
   CancelPendingInteraction();
   if (EquipmentComponent)
   {
      EquipmentComponent->RequestStopFireActiveWeapon();
   }

   if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
   {
      ASC->CancelAllAbilities();
      ASC->AddLooseGameplayTag(FBaruGameplayTags::Get().State_DBNO);
      
      // 09.11 DBNO 세부 로직 추가
      ASC->AddLooseGameplayTag(FBaruGameplayTags::Get().State_Immune);
   }

   // 상태 확정. 복제되어 각 클라의 HandleDBNOStatusChanged 를 깨웁니다.
   BaruPS->SetDBNOState(true);

   // 방치되면 사망
   if (UWorld* World = GetWorld())
   {
      World->GetTimerManager().SetTimer(
         BleedOutTimerHandle, this, &ABaruCharacter::OnBleedOutExpired, BleedOutDuration, false);
      
      // 09.11 DBNO 세부 로직 추가. 2.5초 후 무적 해제 타이머 시작
      World->GetTimerManager().SetTimer(
          DBNOImmunityTimerHandle, this, &ABaruCharacter::EndDBNOImmunity, DBNOImmunityDuration, false);
   }

   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Character %s entered DBNO. Causer: %s (bleed out %.0fs)"),
     *GetName(), DownCauser ? *DownCauser->GetName() : TEXT("None"), BleedOutDuration);
}

// 09.11 DBNO 세부 로직 추가. 2.5초 무적 해제 콜백
void ABaruCharacter::EndDBNOImmunity()
{
   if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
   {
      ASC->RemoveLooseGameplayTag(FBaruGameplayTags::Get().State_Immune);
      BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Character %s DBNO immunity expired."), *GetName());
   }
}

// 09.11 DBNO 세부 로직 추가. 다운 상태에서 맞았을 때 즉사 대신 출혈 시간 단축
void ABaruCharacter::NotifyHitWhileDBNO(float DamageAmount, AActor* Attacker)
{
   if (!HasAuthority() || bIsDead) return;

   LastKiller = Attacker;

   if (UWorld* World = GetWorld())
   {
      const float RemainingTime = World->GetTimerManager().GetTimerRemaining(BleedOutTimerHandle);
      const float NewTime = RemainingTime - DBNODamageBleedReduction;

      BARU_NET_LOG(this, LogBaruCombat, Warning, 
          TEXT("Character %s hit while DBNO! BleedOut reduced: %.1fs -> %.1fs"), *GetName(), RemainingTime, NewTime);

      // 남은 시간이 0 이하면 출혈사(완전 사망) 처리
      if (NewTime <= 0.0f)
      {
         World->GetTimerManager().ClearTimer(BleedOutTimerHandle);
         OnBleedOutExpired();
      }
      else
      {
         // 잔여 시간 갱신
         World->GetTimerManager().SetTimer(
             BleedOutTimerHandle, this, &ABaruCharacter::OnBleedOutExpired, NewTime, false);
      }
   }
}


// ★[추가 09.10] 소생. 서버 전용.
//   HealthRatio 만큼 체력을 회복시키며 일어납니다.
void ABaruCharacter::ReviveFromDBNO(float HealthRatio)
{
   if (!HasAuthority() || bIsDead)
   {
      return;
   }

   ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>();
   if (!BaruPS || !BaruPS->IsDBNO())
   {
      return;
   }

   if (UWorld* World = GetWorld())
   {
      World->GetTimerManager().ClearTimer(BleedOutTimerHandle);
   }

   if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
   {
      ASC->RemoveLooseGameplayTag(FBaruGameplayTags::Get().State_DBNO);

      const float MaxHP = ASC->GetNumericAttribute(UBaruCoreAttributeSet::GetMaxHealthAttribute());
      ASC->SetNumericAttributeBase(
         UBaruCoreAttributeSet::GetHealthAttribute(),
         FMath::Max(1.0f, MaxHP * FMath::Clamp(HealthRatio, 0.01f, 1.0f)));
   }

   LastKiller = nullptr;
   BaruPS->SetDBNOState(false);

   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Character %s revived."), *GetName());
}

// [추가] 블리드아웃 만료 → 완전 사망
void ABaruCharacter::OnBleedOutExpired()
{
   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Character %s bled out."), *GetName());

   // Die_Implementation 을 직접 부르지 않고 인터페이스로 호출합니다.
   // BP 에서 오버라이드했을 때도 반영되도록 하기 위함입니다.
   if (Implements<UCombatInterface>())
   {
      ICombatInterface::Execute_Die(this, LastKiller);
   }
}

// ★[추가 09.10] DBNO 상태 변화 반영. 서버·클라 공통.
//   PlayerState 의 OnRep 이 브로드캐스트하므로 모든 머신에서 호출됩니다.
void ABaruCharacter::HandleDBNOStatusChanged(bool bNewDBNO)
{
   if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
   {
      if (bNewDBNO)
      {
         // 지금은 이동을 완전히 막습니다.
         // 기어가는 연출을 넣게 되면 DisableMovement 대신
         // UpdateMaxWalkSpeed 에서 낮은 속도를 주는 방식으로 바꾸면 됩니다.
         MoveComp->StopMovementImmediately();
         MoveComp->DisableMovement();
      }
      else
      {
         MoveComp->SetMovementMode(MOVE_Walking);
         UpdateMaxWalkSpeed();
      }
   }
   // [추가] 다운 중에만 상호작용 트레이스에 걸리게 합니다.
   //   Interaction 채널 기본 응답이 Overlap 이라, Block 으로 바꿔야
   //   LineTraceSingleByChannel 이 이 캐릭터를 히트로 잡습니다.
   //   살아있는 동안 Block 이면 팀원 뒤의 아이템을 주울 수 없게 됩니다.
   if (UCapsuleComponent* Capsule = GetCapsuleComponent())
   {
      Capsule->SetCollisionResponseToChannel(
         InteractionTraceChannel,
         bNewDBNO ? ECR_Block : ECR_Overlap);
   }
   
   OnDBNOCosmetic(bNewDBNO);   // 몽타주·포스트프로세스는 BP 에서

   BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("DBNO status changed: %d"), bNewDBNO);
}
void ABaruCharacter::Die_Implementation(AActor* Killer)
{
   // 09.11 DBNO 세부 로직 추가 및 정리
   if (bIsDead || !HasAuthority()) return;

   // [09.13] 사망 시 회복 즉시 중단
   StopHealthRegen();
   
   bIsDead = true;
   CancelPendingInteraction();
   LastKiller = Killer;
   
   // [추가] 다운 상태에서 죽었다면 블리드아웃 타이머와 태그를 정리
   if (UWorld* World = GetWorld())
   {
      World->GetTimerManager().ClearTimer(BleedOutTimerHandle);
      
      // 09.11 DBNO 세부 로직 추가
      World->GetTimerManager().ClearTimer(DBNOImmunityTimerHandle);
   }
   if (UAbilitySystemComponent* DBNOASC = GetAbilitySystemComponent())
   {
      DBNOASC->RemoveLooseGameplayTag(FBaruGameplayTags::Get().State_DBNO);
      
      // 09.11 DBNO 세부 로직 추가
      DBNOASC->RemoveLooseGameplayTag(FBaruGameplayTags::Get().State_Immune);
   }
   
   // PlayerState 에 사망 상태 기록
   if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      BaruPS->SetDBNOState(false); // 사망 시 다운 상태 플래그 해제
      BaruPS->SetDeadState(true, Killer);
   }
   
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


bool ABaruCharacter::IsDead_Implementation() const
{
   return bIsDead;
}


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
   // [09.13] 피격 시 기존 회복을 중단하고 10초 대기 타이머 리셋
   if (HasAuthority() && !bIsDead && !Execute_IsDBNO(this))
   {
      StartHealthRegenDelay();
   }
   
   
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

   // ★[추가 09.10] 대상이 요구하는 홀드 시간을 확인합니다.
   //   0 이하면 즉시 실행(아이템 줍기 등), 0보다 크면 그 시간만큼 F 를 누르고 있어야 합니다.
   const float HoldDuration = IInteractableInterface::Execute_GetInteractionDuration(ClaimedActor);

   if (HoldDuration <= 0.0f)
   {
      // [09.13] F키를 뗄 때 EndInteraction을 호출할 수 있도록 즉시 실행 대상도 타깃으로 캐싱
      CancelPendingInteraction();
      PendingInteractTarget = ClaimedActor;

      IInteractableInterface::Execute_ExecuteInteraction(ClaimedActor, this);
      BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Interaction executed on: %s"), *ClaimedActor->GetName());
      return;
   }

   // 홀드 상호작용 시작. 이전에 잡고 있던 게 있으면 먼저 정리합니다.
   CancelPendingInteraction();
   PendingInteractTarget = ClaimedActor;

   GetWorldTimerManager().SetTimer(
      InteractionTimerHandle,
      this,
      &ABaruCharacter::CompletePendingInteraction,
      HoldDuration,
      false);

   BARU_NET_LOG(this, LogBaruItem, Log,
      TEXT("Interaction hold started: %s (%.2fs)"), *ClaimedActor->GetName(), HoldDuration);
}

// [추가] 홀드 시간이 다 찼을 때 서버에서 실행됩니다.
//   시작 시점의 검증만 믿지 않고 다시 확인합니다.
//   누르고 있는 동안 플레이어가 멀어졌거나 문이 잠겼을 수 있기 때문입니다.
void ABaruCharacter::CompletePendingInteraction()
{
   AActor* Target = PendingInteractTarget.Get();
   CancelPendingInteraction();   // 타이머·타깃 먼저 정리

   if (bIsDead || !IsValid(Target))
   {
      return;
   }

   const float MaxDist = InteractionTraceDistance + InteractionLagTolerance;
   if (FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(MaxDist))
   {
      BARU_NET_LOG(this, LogBaruNet, Warning,
         TEXT("Interaction hold cancelled: target out of range (%s)"), *Target->GetName());
      return;
   }

   if (!IInteractableInterface::Execute_CanInteract(Target, this))
   {
      BARU_NET_LOG(this, LogBaruItem, Log,
         TEXT("Interaction hold refused by target: %s"), *Target->GetName());
      return;
   }

   IInteractableInterface::Execute_ExecuteInteraction(Target, this);
   BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Interaction executed on: %s"), *Target->GetName());
}

// ★[추가] 진행 중인 홀드를 정리 서버 전용.
void ABaruCharacter::CancelPendingInteraction()
{
   if (UWorld* World = GetWorld())
   {
      World->GetTimerManager().ClearTimer(InteractionTimerHandle);
   }
   PendingInteractTarget.Reset();
}

// ★[추가] 클라이언트가 F 를 뗐을 때 서버가 홀드를 중단
bool ABaruCharacter::Server_StopInteraction_Validate()
{
   return true;
}

void ABaruCharacter::Server_StopInteraction_Implementation()
{
   // [09.13] F키를 뗄 때 누르고 있던 대상(버튼 등)에게 상호작용 종료 통보
   if (PendingInteractTarget.IsValid())
   {
      AActor* Target = PendingInteractTarget.Get();
      if (Target && Target->Implements<UInteractableInterface>())
      {
         IInteractableInterface::Execute_EndInteraction(Target, this);
      }

      BARU_NET_LOG(this, LogBaruItem, Log,
          TEXT("Interaction hold cancelled by input release: %s"), *Target->GetName());
   }
   CancelPendingInteraction();
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

// [09.13] 체력 자연 회복 로직 추가
// 10초 대기 타이머 시작 (피격마다 호출되어 타이머가 갱신됨)
void ABaruCharacter::StartHealthRegenDelay()
{
    if (!HasAuthority()) return;

    StopHealthRegen();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            HealthRegenDelayTimerHandle,
            this,
            &ABaruCharacter::OnHealthRegenDelayExpired,
            HealthRegenDelay,
            false
        );
    }
}

// 10초간 추가 피격이 없었을 때 회복 틱 시작
void ABaruCharacter::OnHealthRegenDelayExpired()
{
    if (!HasAuthority() || bIsDead || Execute_IsDBNO(this)) return;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            HealthRegenTickTimerHandle,
            this,
            &ABaruCharacter::TickHealthRegen,
            HealthRegenTickInterval,
            true
        );
    }
}

// 회복 틱: 최대 80.0f 한도까지 체력 점진 회복
void ABaruCharacter::TickHealthRegen()
{
    if (!HasAuthority() || bIsDead || Execute_IsDBNO(this))
    {
        StopHealthRegen();
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    const float CurrentHealth = ASC->GetNumericAttribute(UBaruCoreAttributeSet::GetHealthAttribute());
    const float MaxHealth = ASC->GetNumericAttribute(UBaruCoreAttributeSet::GetMaxHealthAttribute());
    const float TargetLimit = FMath::Min(MaxRegenHealth, MaxHealth);

    // 80 이상 도달 시 회복 종료
    if (CurrentHealth >= TargetLimit)
    {
        StopHealthRegen();
        return;
    }

    const float HealAmount = HealthRegenRatePerSecond * HealthRegenTickInterval;
    const float NewHealth = FMath::Min(CurrentHealth + HealAmount, TargetLimit);

    ASC->SetNumericAttributeBase(UBaruCoreAttributeSet::GetHealthAttribute(), NewHealth);
}

// 모든 회복 타이머 초기화
void ABaruCharacter::StopHealthRegen()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HealthRegenDelayTimerHandle);
        World->GetTimerManager().ClearTimer(HealthRegenTickTimerHandle);
    }
}

// [09.13] Early-Out 구조의 반동 회복 틱
void ABaruCharacter::Tick(float DeltaSeconds)
{
   Super::Tick(DeltaSeconds);

   // 로컬 클라이언트만 연산 (서버 및 원격 프록시 스킵)
   if (!IsLocallyControlled())
   {
      return;
   }
   
   // 1. 총기 반동 회복 (남은 반동이 있을 때만 가산)
   if (RemainingRecoilRecoveryPitch > 0.0f)
   {
      const float RecoveryDelta = FMath::Min(RemainingRecoilRecoveryPitch, CurrentRecoilRecoverySpeed * DeltaSeconds);
      AddControllerPitchInput(RecoveryDelta);
      RemainingRecoilRecoveryPitch -= RecoveryDelta;

      if (RemainingRecoilRecoveryPitch <= 0.0f)
      {
         RemainingRecoilRecoveryPitch = 0.0f;
      }
   }

   // 2. 조준 집중 연출 보간 (FOV, 터널비전 비네팅, 색수차)
   UpdateAimingEffects(DeltaSeconds);
}

// [09.13] 사격 시 카메라 킥 및 복구량 계산
void ABaruCharacter::ApplyRecoil(const FBaruRecoilData& InRecoilData)
{
   if (!IsLocallyControlled() || !Controller)
   {
      return;
   }

   const float PitchKick = FMath::RandRange(InRecoilData.MinPitchRecoil, InRecoilData.MaxPitchRecoil);
   const float YawKick = FMath::RandRange(InRecoilData.MinYawRecoil, InRecoilData.MaxYawRecoil);

   // 카메라 즉각 킥 (-Pitch: 상향 앙각, Yaw: 좌우 수평 흔들림)
   AddControllerPitchInput(-PitchKick);
   AddControllerYawInput(YawKick);

   // 복구 속도가 0보다 클 때만 복구 수치 누적
   if (InRecoilData.RecoilRecoverySpeed > 0.0f)
   {
      RemainingRecoilRecoveryPitch += PitchKick;
      CurrentRecoilRecoverySpeed = InRecoilData.RecoilRecoverySpeed;
   }
}

// [09.13] 조준 기능 추가
void ABaruCharacter::Input_AimStart()
{
   if (bIsDead || Execute_IsDBNO(this)) return;
   SetAiming(true);
}

void ABaruCharacter::Input_AimStop()
{
   SetAiming(false);
}

void ABaruCharacter::SetAiming(bool bNewAiming)
{
   bIsAiming = bNewAiming;
   CurrentTargetFOV = bIsAiming ? AimFOV : DefaultFOV;
   CurrentTargetVignette = bIsAiming ? AimVignette : DefaultVignette;
   CurrentTargetFringe = bIsAiming ? AimFringe : DefaultFringe;
}

void ABaruCharacter::UpdateAimingEffects(float DeltaSeconds)
{
   if (!FollowCamera)
   {
      return;
   }

   // FOV 보간 (목표값 오차 0.05도 이내 도달 시 연산 중단)
   const float CurrentFOV = FollowCamera->FieldOfView;
   if (!FMath::IsNearlyEqual(CurrentFOV, CurrentTargetFOV, 0.05f))
   {
      FollowCamera->SetFieldOfView(FMath::FInterpTo(CurrentFOV, CurrentTargetFOV, DeltaSeconds, AimInterpSpeed));
   }
   else if (CurrentFOV != CurrentTargetFOV)
   {
      FollowCamera->SetFieldOfView(CurrentTargetFOV);
   }

   // 터널비전 비네팅 보간 (오차 0.005 이내 도달 시 연산 중단)
   float& CurrentVignette = FollowCamera->PostProcessSettings.VignetteIntensity;
   if (!FMath::IsNearlyEqual(CurrentVignette, CurrentTargetVignette, 0.005f))
   {
      CurrentVignette = FMath::FInterpTo(CurrentVignette, CurrentTargetVignette, DeltaSeconds, AimInterpSpeed);
   }
   else if (CurrentVignette != CurrentTargetVignette)
   {
      CurrentVignette = CurrentTargetVignette;
   }

   // 렌즈 외곽 색수차 왜곡 보간 (오차 0.005 이내 도달 시 연산 중단)
   float& CurrentFringe = FollowCamera->PostProcessSettings.SceneFringeIntensity;
   if (!FMath::IsNearlyEqual(CurrentFringe, CurrentTargetFringe, 0.005f))
   {
      CurrentFringe = FMath::FInterpTo(CurrentFringe, CurrentTargetFringe, DeltaSeconds, AimInterpSpeed);
   }
   else if (CurrentFringe != CurrentTargetFringe)
   {
      CurrentFringe = CurrentTargetFringe;
   }
}