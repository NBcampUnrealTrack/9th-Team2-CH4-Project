#include "Player/BaruPlayerState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"                                  
#include "AbilitySystem/BaruAbilitySystemComponent.h"             
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "Core/BaruLobbyGameMode.h"   // 추가 준비 상태 변경 시 로비 재평가 요청
#include "Components/BaruHealthComponent.h"                       
#include "Gameplay/Inventory/BaruInventoryComponent.h"          
#include "BaruLog.h"

ABaruPlayerState::ABaruPlayerState()
{
    PrimaryActorTick.bCanEverTick = false;
    
    bReplicates = true;
    SetNetUpdateFrequency(10.0f);

    // GAS Components
    AbilitySystemComponent = CreateDefaultSubobject<UBaruAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // AttributeSet
    CoreAttributeSet = CreateDefaultSubobject<UBaruCoreAttributeSet>(TEXT("CoreAttributeSet"));
    PlayerAttributeSet = CreateDefaultSubobject<UBaruPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
    HealthComponent = CreateDefaultSubobject<UBaruHealthComponent>(TEXT("HealthComponent"));
    InventoryComponent = CreateDefaultSubobject<UBaruInventoryComponent>(TEXT("InventoryComponent"));       // 아이템 inven 요청
}


void ABaruPlayerState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (!AbilitySystemComponent)
    {
        return;
    }

    if (HealthComponent)
    {
        HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
    }

    SanityChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        UBaruPlayerAttributeSet::GetSanityAttribute()).AddUObject(this, &ABaruPlayerState::HandleSanityChanged);
}

// [추가] 델리게이트 해제
void ABaruPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
            UBaruPlayerAttributeSet::GetSanityAttribute()).Remove(SanityChangedHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void ABaruPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABaruPlayerState, bIsReady);
    DOREPLIFETIME(ABaruPlayerState, bIsDBNO);
    DOREPLIFETIME(ABaruPlayerState, bIsDead);
    DOREPLIFETIME(ABaruPlayerState, MonsterKillCount);
}

// [추가] 레벨 이동 시 값 인수인계
void ABaruPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    if (ABaruPlayerState* NewPS = Cast<ABaruPlayerState>(PlayerState))
    {
        // 새 레벨 진입 시 레디 상태 초기화
        NewPS->bIsReady = false;

        // // TODO : 인벤토리 서버 전용 함수 구현 시 주석 해제하여 데이터 인수인계
        // if (InventoryComponent && NewPS->InventoryComponent)
        // {
        //     NewPS->InventoryComponent->CopyInventoryFrom(InventoryComponent);
        // }
    }
}

UAbilitySystemComponent* ABaruPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

// [수정] 헤더 인라인에서 .cpp 로 이동. 동작은 기존과 동일합니다.
float ABaruPlayerState::GetHealth() const
{
    return CoreAttributeSet ? CoreAttributeSet->GetHealth() : 0.0f;
}

float ABaruPlayerState::GetMaxHealth() const
{
    return CoreAttributeSet ? CoreAttributeSet->GetMaxHealth() : 0.0f;
}

// [수정] 삭제한 복제 변수 대신 AttributeSet 값을 반환 (단일 소스 원칙)
float ABaruPlayerState::GetSanity() const
{
    return PlayerAttributeSet ? PlayerAttributeSet->GetSanity() : 0.0f;
}

// Server RPC
bool ABaruPlayerState::Server_SetReadyStatus_Validate(bool bNewReady)
{
    return true;
}

void ABaruPlayerState::Server_SetReadyStatus_Implementation(bool bNewReady)
{
    if (bIsReady == bNewReady)
    {
       return;
    }

    bIsReady = bNewReady;
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Ready Status Changed: %d"), bIsReady);
    
    OnReadyStatusChanged.Broadcast(bIsReady);
    
    //[추가]
    if (ABaruLobbyGameMode* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABaruLobbyGameMode>() : nullptr)
    {
        LobbyGM->OnPlayerReadyStatusChanged();
    }
}

bool ABaruPlayerState::Server_UpdateNickname_Validate(const FString& NewNickname)
{
    return !NewNickname.TrimStartAndEnd().IsEmpty() && NewNickname.Len() <= 24;
}

void ABaruPlayerState::Server_UpdateNickname_Implementation(const FString& NewNickname)
{
    const FString TrimmedName = NewNickname.TrimStartAndEnd();
    SetPlayerName(TrimmedName);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Nickname Updated to: %s"), *TrimmedName);
}

// [Server Only] 상태 변경 함수
void ABaruPlayerState::SetDBNOState(bool bNewDBNO)
{
    if (!HasAuthority())
    {
       return;
    }

    if (bIsDBNO != bNewDBNO)
    {
       bIsDBNO = bNewDBNO;
       OnRep_IsDBNO(); // Listen Server 환경 및 서버 로직 즉시 갱신
    }
}

// [추가] 사망 상태 확정/해제 (서버 전용)
void ABaruPlayerState::SetDeadState(bool bNewDead, AActor* Killer)
{
    if (!HasAuthority() || bIsDead == bNewDead)
    {
        return;
    }

    bIsDead = bNewDead;
    OnRep_IsDead();   // 서버에서는 OnRep 이 자동 호출되지 않으므로 직접 호출

    //  HealthComponent 의 OnDeath 델리게이트는 지금까지 선언만 되고
    //  어디서도 브로드캐스트되지 않는 죽은 델리게이트였습니다. 여기서 연결합니다.
    if (bNewDead && HealthComponent)
    {
        HealthComponent->NotifyDeath(Killer);
    }
}

// [수정] 복제 변수 직접 대입 → GAS 어트리뷰트에 반영.
//   함수 시그니처는 그대로 유지했습니다. ABaruTestGameMode::RespawnPlayer 가 이 함수를
//   호출하고 있어서 지우거나 바꾸면 제 권한 밖 파일이 컴파일되지 않습니다.
void ABaruPlayerState::SetSanityValue(float NewSanity)
{
    if (!HasAuthority() || !AbilitySystemComponent)
    {
       return;
    }

    const float MaxValue = PlayerAttributeSet ? PlayerAttributeSet->GetMaxSanity() : 100.0f;
    const float Clamped = FMath::Clamp(NewSanity, 0.0f, MaxValue);

    // 주의: 이건 리셋/디버그용 직접 대입
    //  인게임 정신력 증감은 반드시 GameplayEffect 로 처리해야 함
    AbilitySystemComponent->SetNumericAttributeBase(UBaruPlayerAttributeSet::GetSanityAttribute(), Clamped);
}

// [추가] 어트리뷰트 변경 → UI 델리게이트
void ABaruPlayerState::HandleSanityChanged(const FOnAttributeChangeData& ChangeData)
{
    BARU_NET_LOG(this, LogBaruSanity, Verbose, TEXT("Sanity Changed: %.1f"), ChangeData.NewValue);
    OnSanityChanged.Broadcast(ChangeData.NewValue);
}

// OnRep
void ABaruPlayerState::OnRep_IsReady()
{
    BARU_NET_LOG(this, LogBaruSession, Verbose, TEXT("OnRep_IsReady: %d"), bIsReady);
    OnReadyStatusChanged.Broadcast(bIsReady);
}

void ABaruPlayerState::OnRep_IsDBNO()
{
    BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("OnRep_IsDBNO: %d"), bIsDBNO);
    OnDBNOStatusChanged.Broadcast(bIsDBNO);
}

// [추가]
void ABaruPlayerState::OnRep_IsDead()
{
    BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("OnRep_IsDead: %d"), bIsDead);
    OnDeadStatusChanged.Broadcast(bIsDead);
}

void ABaruPlayerState::AddMonsterKill()
{
    if (!HasAuthority())
    {
        return;
    }

    MonsterKillCount++;
    BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("Player %s MonsterKillCount: %d"), *GetPlayerName(), MonsterKillCount);

    OnRep_MonsterKillCount();
}

void ABaruPlayerState::OnRep_MonsterKillCount()
{
    OnMonsterKillCountChanged.Broadcast(MonsterKillCount);
}