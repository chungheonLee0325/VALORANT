#include "Phoenix_Q_HotHands.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerState.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AgentAbility/BaseProjectile.h"

UPhoenix_Q_HotHands::UPhoenix_Q_HotHands() : UBaseGameplayAbility()
{
    // === 어빌리티 기본 설정 ===
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
    SetAssetTags(Tags);

    m_AbilityID = 2002;
    InputType = EAbilityInputType::MultiPhase;
    
    // === 후속 입력 설정 (CDO에서 안전한 방식) ===
    ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
    ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));
    
    // === 하위 호환성을 위한 기존 방식도 유지 ===
    // FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
    // FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));
    
    // === 타임아웃 설정 ===
    PhaseTimeoutDuration = 5.0f;
    
    // === 차단 태그 설정 - 스킬 사용 중 다른 행동 제한 ===
    // BlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Block.Movement")));
    
    // === 필요한 태그 설정 (예: 특정 상황에서만 사용 가능) ===
    // RequiredTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Alive")));
    
    // === 어빌리티 상태 태그 초기화 ===
    //AbilityStateTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Phoenix.Q")));
}

void UPhoenix_Q_HotHands::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // 부모 클래스에서 MultiPhase 타입 처리 (후속 입력 자동 등록됨)
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q 스킬 활성화됨"));
}

void UPhoenix_Q_HotHands::HandleLeftClick(FGameplayEventData EventData)
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 좌클릭 처리 (직선 던지기)"));
    
    // 준비 상태일 때만 실행 가능
    if (CurrentPhase == EAbilityPhase::Ready)
    {
        CurrentThrowType = EPhoenixQThrowType::Straight;
        
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 던지기 실행"));
        
        // 실행 단계로 전환
        SetupPhase(EAbilityPhase::Executing);
        
        // 직선 던지기 실행
        ExecuteStraightThrow();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 준비 상태가 아니므로 좌클릭 무시됨. 현재 단계: %d"), (int32)CurrentPhase);
    }
    
    // 하위 호환성을 위해 기존 메서드도 호출
    Active_Left_Click(EventData);
}

void UPhoenix_Q_HotHands::HandleRightClick(FGameplayEventData EventData)
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 우클릭 처리 (포물선 던지기)"));
    
    // 준비 상태일 때만 실행 가능
    if (CurrentPhase == EAbilityPhase::Ready)
    {
        CurrentThrowType = EPhoenixQThrowType::Curved;
        
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 던지기 실행"));
        
        // 실행 단계로 전환
        SetupPhase(EAbilityPhase::Executing);
        
        // 포물선 던지기 실행
        ExecuteCurvedThrow();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 준비 상태가 아니므로 우클릭 무시됨. 현재 단계: %d"), (int32)CurrentPhase);
    }
}

void UPhoenix_Q_HotHands::ExecuteStraightThrow()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 던지기 실행"));
    
    // 직선 던지기 애니메이션 재생
    if (ThrowMontage && CachedActorInfo.GetAnimInstance())
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, 
            "StraightThrowAnimation", 
            ThrowMontage, 
            1.0f
        );
        
        if (Task)
        {
            Task->OnCompleted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
            Task->OnInterrupted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
            Task->ReadyForActivation();
        }
    }
    
    // 직선 던지기 사운드 재생
    if (ThrowSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
    }
    
    // 핸드 이펙트 중지
    StopHandFireEffect();
    
    // 직선 투사체 생성
    if (SpawnProjectileByType(EPhoenixQThrowType::Straight))
    {
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 투사체 생성 성공"));
        
        // 쿨다운 단계로 전환
        SetupPhase(EAbilityPhase::Cooldown);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 직선 투사체 생성 실패"));
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
    }
}

void UPhoenix_Q_HotHands::ExecuteCurvedThrow()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 던지기 실행"));
    
    // 포물선 던지기 애니메이션 재생
    if (ThrowMontage && CachedActorInfo.GetAnimInstance())
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, 
            "CurvedThrowAnimation", 
            ThrowMontage, 
            1.0f
        );
        
        if (Task)
        {
            Task->OnCompleted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
            Task->OnInterrupted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
            Task->ReadyForActivation();
        }
    }
    
    // 포물선 던지기 사운드 재생
    if (ThrowSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
    }
    
    // 핸드 이펙트 중지
    StopHandFireEffect();
    
    // 포물선 투사체 생성
    if (SpawnProjectileByType(EPhoenixQThrowType::Curved))
    {
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 투사체 생성 성공"));
        
        // 쿨다운 단계로 전환
        SetupPhase(EAbilityPhase::Cooldown);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 포물선 투사체 생성 실패"));
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
    }
}

bool UPhoenix_Q_HotHands::SpawnProjectileByType(EPhoenixQThrowType ThrowType)
{
    TSubclassOf<ABaseProjectile> ProjectileToSpawn = nullptr;
    
    switch (ThrowType)
    {
        case EPhoenixQThrowType::Straight:
            // ProjectileToSpawn = StraightProjectileClass ? StraightProjectileClass : ProjectileClass;
            break;
        case EPhoenixQThrowType::Curved:
            // ProjectileToSpawn = CurvedProjectileClass ? CurvedProjectileClass : ProjectileClass;
            break;
        default:
            ProjectileToSpawn = ProjectileClass;
            break;
    }
        ProjectileToSpawn = ProjectileClass;
    
    if (ProjectileToSpawn == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 투사체 클래스가 설정되지 않음"));
        return false;
    }

    // 기존 SpawnProjectile 로직을 사용하되, 투사체 클래스만 교체
    TSubclassOf<ABaseProjectile> OriginalProjectileClass = ProjectileClass;
    ProjectileClass = ProjectileToSpawn;
    
    bool bResult = SpawnProjectile(CachedActorInfo);
    
    // 원래 클래스로 복원
    ProjectileClass = OriginalProjectileClass;
    
    return bResult;
}

void UPhoenix_Q_HotHands::PlayReadyEffects()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 준비 이펙트 재생"));
    
    // 준비 사운드 재생
    if (ReadySound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), ReadySound);
    }
    
    // 핸드 파이어 이펙트 시작
    StartHandFireEffect();
    
    // 이동 차단 적용
    if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
    {
        ASC->SetAbilityState(FGameplayTag::RequestGameplayTag(FName("Block.Movement")), true);
    }
    
    // 블루프린트 이벤트 호출
    OnReadyEffectStarted();
}

void UPhoenix_Q_HotHands::PlayExecuteEffects()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 실행 이펙트 재생"));
    
    // 실행 관련 이펙트들은 각 던지기 타입에서 처리됨
}

void UPhoenix_Q_HotHands::PlayCooldownEffects()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 쿨다운 이펙트 재생"));
    
    // 이미 핸드 이펙트는 중지됨
    // 추가적인 쿨다운 이펙트가 있다면 여기서 처리
}

void UPhoenix_Q_HotHands::ExecutePhaseAction(float HoldTime)
{
    // 블루프린트에서 호출 가능한 단계별 액션
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 단계 액션 실행, HoldTime: %f"), HoldTime);
}

void UPhoenix_Q_HotHands::StartHandFireEffect()
{
    if (HandFireEffect && CachedActorInfo.OwnerActor.IsValid())
    {
        // 플레이어의 손 위치에 파티클 이펙트 생성
        if (ABaseAgent* Agent = Cast<ABaseAgent>(CachedActorInfo.OwnerActor.Get()))
        {
            // 적절한 소켓이나 위치에 파티클 컴포넌트 생성
            HandFireComponent = UGameplayStatics::SpawnEmitterAttached(
                HandFireEffect,
                Agent->GetMesh(),
                FName("hand_rSocket"), // 실제 손 소켓 이름으로 변경 필요
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::SnapToTarget,
                true
            );
            
            if (HandFireComponent)
            {
                UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 핸드 파이어 이펙트 생성됨"));
            }
        }
    }
}

void UPhoenix_Q_HotHands::StopHandFireEffect()
{
    if (HandFireComponent && IsValid(HandFireComponent))
    {
        HandFireComponent->DestroyComponent();
        HandFireComponent = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 핸드 파이어 이펙트 중지됨"));
    }
}

void UPhoenix_Q_HotHands::OnThrowAnimationCompleted()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 발사 애니메이션 완료"));
    
    // 애니메이션 완료 후 추가 처리가 필요하다면 여기서
}

void UPhoenix_Q_HotHands::OnReadyEffectStarted()
{
    // 블루프린트에서 오버라이드 가능한 이벤트
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 준비 이펙트 시작 이벤트"));
}

void UPhoenix_Q_HotHands::CleanupAbility()
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 어빌리티 정리"));
    
    // 핸드 파이어 이펙트 정리
    StopHandFireEffect();
    
    // 차단 태그 제거
    if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
    {
    }
    
    // 던지기 타입 초기화
    CurrentThrowType = EPhoenixQThrowType::None;
    
    // 부모 클래스의 정리 작업 수행
    Super::CleanupAbility();
}

void UPhoenix_Q_HotHands::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       bool bReplicateCancelAbility)
{
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 스킬 취소"));
    
    // 취소 애니메이션 재생
    if (CancelMontage && CachedActorInfo.GetAnimInstance())
    {
        UAnimInstance* AnimInstance = CachedActorInfo.GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->Montage_Play(CancelMontage);
        }
    }
    
    // 부모 클래스의 취소 처리
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

// === 하위 호환성을 위한 기존 메서드 ===
void UPhoenix_Q_HotHands::Active_Left_Click(FGameplayEventData data)
{
    // 새로운 HandleLeftClick에서 이미 처리되므로 여기서는 로그만
    UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 기존 Active_Left_Click 호출됨 (하위 호환성)"));
}