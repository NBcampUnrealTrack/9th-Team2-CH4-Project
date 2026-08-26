#include "Character/BaruCharacter.h" 
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Player/BaruPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Components/BaruHealthComponent.h"

ABaruCharacter::ABaruCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1인칭 카메라 설정 
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(RootComponent);
    FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f)); // 눈높이 위치 (Z: 60)
    FollowCamera->bUsePawnControlRotation = true; // 마우스 회전에 따라 카메라 회전

    // 1인칭 캐릭터 회전 제어 
    bUseControllerRotationYaw = true; // 마우스 좌우 회전 시 캐릭터 몸통도 함께 회전
    GetCharacterMovement()->bOrientRotationToMovement = false; 
    
}

// [서버 전용] 플레이어가 컨트롤러에 빙의될 때 호출
void ABaruCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 서버 측 GAS ActorInfo 초기화
    InitAbilityActorInfo();
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
   if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
   {
      if (UAbilitySystemComponent* ASC = BaruPS->GetAbilitySystemComponent())
      {
         // 캐릭터와 PlayerState를 GAS에 연결
         ASC->InitAbilityActorInfo(BaruPS, this);
          
         
         if (UBaruHealthComponent* PSHealthComp = BaruPS->GetHealthComponent())
         {
            PSHealthComp->InitializeWithAbilitySystem(ASC);
         }
      }
   }
}

void ABaruCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ABaruCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // [멀티플레이 필수 수정] 로컬 컨트롤러일 때만 LocalPlayerSubsystem에 접근 (서버 크래시 방지)
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
       if (PC->IsLocalController())
       {
          if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
          {
             if (DefaultMappingContext)
             {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
             }
          }
       }
    }

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
          EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
          EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
       }
    }
}

void ABaruCharacter::Move(const FInputActionValue& Value)
{
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