#include "Character/BaruCharacter.h" 
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h" // [추가] 1인칭 메쉬 제어용 필수 헤더
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Player/BaruPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Components/BaruHealthComponent.h"
#include "DrawDebugHelpers.h"
#include "BaruLog.h"

ABaruCharacter::ABaruCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1인칭 카메라 설정 
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(RootComponent);
    FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f)); // 눈높이 위치 (Z: 60)
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

void ABaruCharacter::OnRep_Controller()
{
   Super::OnRep_Controller();
    
   // 클라이언트 측에 컨트롤러가 동기화된 시점에 GAS ActorInfo 갱신
   InitAbilityActorInfo();
}

// CombatInterface 함수 구현부 
void ABaruCharacter::Die_Implementation(AActor* Killer)
{
    // [TODO] 사망 태그 부여 및 래그돌/사망 애니메이션 처리 위치
    BARU_LOG(LogBaruCombat, Log, TEXT("Character %s has died. Killer: %s"), *GetName(), Killer ? *Killer->GetName() : TEXT("None"));
}

bool ABaruCharacter::IsDead_Implementation() const
{
    // [수정] PlayerState의 AttributeSet(GAS) 체력 값을 조회하여 사망 판단
    if (const ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
    {
        return BaruPS->GetHealth() <= 0.0f;
    }
    return false;
}

bool ABaruCharacter::PerformLineTrace(FHitResult& OutHitResult, float TraceDistance)
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

   // 자기 자신 및 소유한 액터는 트레이스에서 제외
   FCollisionQueryParams TraceParams(FName(TEXT("BaruLineTrace")), true, this);
   TraceParams.bReturnPhysicalMaterial = true;

   // ECC_Visibility 채널 기반 트레이스 실행
   const bool bHit = World->LineTraceSingleByChannel(
       OutHitResult,
       TraceStart,
       TraceEnd,
       ECC_Visibility,
       TraceParams
   );

#if WITH_EDITOR
   // 디버그용 라인 그리기 (에디터 전용)
   const FColor LineColor = bHit ? FColor::Green : FColor::Red;
   DrawDebugLine(World, TraceStart, bHit ? OutHitResult.ImpactPoint : TraceEnd, LineColor, false, 2.0f, 0, 1.0f);
#endif

   return bHit;
}

bool ABaruCharacter::Server_ProcessInteraction_Validate(const FHitResult& HitResult)
{
   return true;
}

void ABaruCharacter::Server_ProcessInteraction_Implementation(const FHitResult& HitResult)
{
   // [Server Only] 라인 트레이스에 맞은 대상에게 실제 영향(GAS GE 적용, 상호작용 등)을 주는 로직 작성
   if (AActor* HitActor = HitResult.GetActor())
   {
      BARU_LOG(LogBaruCombat, Log, TEXT("Server Processed Interaction with: %s"), *HitActor->GetName());
   }
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