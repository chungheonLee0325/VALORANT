#include "BaseGameplayAbility.h"

#include <GameManager/SubsystemSteamManager.h>

#include "Valorant.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AgentAbility/BaseProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UBaseGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 어빌리티 시스템 컴포넌트 가져오기
    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
    if (!ASC)
    {
        return false;
    }

    // 태그 기반 활성화 조건 확인
    if (!ASC->CanActivateAbilities())
    {
        UE_LOG(LogTemp, Warning, TEXT("어빌리티 활성화가 차단된 상태입니다."));
        return false;
    }

    // 필요한 태그 확인
    if (!RequiredTags.IsEmpty())
    {
        if (!ASC->HasAllMatchingGameplayTags(RequiredTags))
        {
            UE_LOG(LogTemp, Warning, TEXT("필요한 태그가 부족하여 어빌리티를 활성화할 수 없습니다."));
            return false;
        }
    }

    // 차단 태그 확인
    if (!BlockedTags.IsEmpty())
    {
        if (ASC->HasAnyMatchingGameplayTags(BlockedTags))
        {
            UE_LOG(LogTemp, Warning, TEXT("차단 태그로 인해 어빌리티를 활성화할 수 없습니다."));
            return false;
        }
    }

    // 스택 기반 어빌리티인 경우 스택 확인
    if (m_AbilityID > 0)
    {
        AAgentPlayerState* PS = Cast<AAgentPlayerState>(ActorInfo->PlayerController->PlayerState);
        if (!PS)
        {
            UE_LOG(LogTemp, Error, TEXT("어빌리티 활성화 확인 실패: PlayerState가 NULL입니다."));
            return false;
        }

        int32 CurrentStack = GetAbilityStack(ActorInfo->PlayerController.Get());
        if (CurrentStack <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 없어 활성화할 수 없습니다."), m_AbilityID);
            return false;
        }
    }

    return true;
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedActorInfo = *ActorInfo;
    m_ActorInfo = *ActorInfo;  // 하위 호환성

    SetInputContext(true);

    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (ASC)
    {
        // 어빌리티 실행 상태로 설정
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Executing, true);
        
        // 입력 타입에 따라 다른 초기화 로직
        switch (InputType)
        {
            case EAbilityInputType::Instant:
                HandleInstantAbility();
                break;
                
            case EAbilityInputType::Hold:
                HandleHoldAbility();
                break;
                
            case EAbilityInputType::Toggle:
                HandleToggleAbility();
                break;
                
            case EAbilityInputType::Sequence:
                HandleSequenceAbility();
                break;
                
            case EAbilityInputType::MultiPhase:
                HandleMultiPhaseAbility();
                break;
                
            case EAbilityInputType::Repeatable:
                HandleRepeatableAbility();
                break;
        }
        
        // 어빌리티 시작 이벤트 브로드캐스트
        FGameplayEventData EventData;
        EventData.EventTag = FValorantGameplayTags::Get().Event_Ability_Started;
        BroadcastAbilityEvent(FValorantGameplayTags::Get().Event_Ability_Started, EventData);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GA, ASC가 AgentAbilitySystemComponent를 상속받지 않았습니다."));
    }

    // 현재 상호작용 객체 비활성화 (무기 등)
    if (auto* ps = Cast<AAgentPlayerState>(ActorInfo->OwnerActor))
    {
        auto* agent = Cast<ABaseAgent>(ps->GetPawn());
        if (agent == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("BaseGameplayAbility, Agent Null"));
        }
        
        auto* curInteractor = agent->GetCurrentInterator();
        if (curInteractor)
        {
            NET_LOG(LogTemp, Warning, TEXT("인터랙터 숨기기 %s"), *curInteractor->GetActorNameOrLabel());
            curInteractor->SetActive(false);
            agent->ServerRPC_SetCurrentInteractor(nullptr);
        }
    }
}

void UBaseGameplayAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputPressed(Handle, ActorInfo, ActivationInfo);
    
    // 홀드 기반 스킬 처리
    if (InputType == EAbilityInputType::Hold && CurrentPhase == EAbilityPhase::Ready)
    {
        SetupPhase(EAbilityPhase::Charging);
        HoldStartTime = GetWorld()->GetTimeSeconds();
    }
    
    // 반복 입력 스킬 처리
    if (InputType == EAbilityInputType::Repeatable)
    {
        if (CurrentRepeatCount < MaxRepeatCount)
        {
            ExecutePhaseAction();
            CurrentRepeatCount++;
            
            if (CurrentRepeatCount >= MaxRepeatCount)
            {
                SetupPhase(EAbilityPhase::Cooldown);
            }
        }
    }
    
    // 토글 스킬 처리
    if (InputType == EAbilityInputType::Toggle)
    {
        if (CurrentPhase == EAbilityPhase::Ready)
        {
            SetupPhase(EAbilityPhase::Executing);
        }
        else if (CurrentPhase == EAbilityPhase::Executing)
        {
            SetupPhase(EAbilityPhase::Cooldown);
        }
    }
    
    NET_LOG(LogTemp, Warning, TEXT("스킬 InputPressed"));
}

void UBaseGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputReleased(Handle, ActorInfo, ActivationInfo);
    
    // 홀드형 스킬 릴리즈 처리
    if (InputType == EAbilityInputType::Hold && CurrentPhase == EAbilityPhase::Charging)
    {
        float HoldTime = GetWorld()->GetTimeSeconds() - HoldStartTime;
        
        if (HoldTime >= MinHoldDuration)
        {
            HoldTime = FMath::Min(HoldTime, MaxHoldDuration);
            SetupPhase(EAbilityPhase::Executing);
            ExecutePhaseAction(HoldTime);
        }
        else
        {
            CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        }
    }
    
    NET_LOG(LogTemp, Warning, TEXT("스킬 InputReleased"));
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!bWasCancelled)
    {
        ConsumeAbilityStack(ActorInfo->PlayerController.Get());
    }

    CleanupAbility();
    
    // 어빌리티 종료 이벤트 브로드캐스트
    FGameplayEventData EventData;
    EventData.EventTag = FValorantGameplayTags::Get().Event_Ability_Ended;
    BroadcastAbilityEvent(FValorantGameplayTags::Get().Event_Ability_Ended, EventData);
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    NET_LOG(LogTemp, Warning, TEXT("스킬 EndAbility"));
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         bool bReplicateCancelAbility)
{
    CleanupAbility();
    
    // 어빌리티 취소 이벤트 브로드캐스트
    FGameplayEventData EventData;
    EventData.EventTag = FValorantGameplayTags::Get().Event_Ability_Cancelled;
    BroadcastAbilityEvent(FValorantGameplayTags::Get().Event_Ability_Cancelled, EventData);
    
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
    NET_LOG(LogTemp, Warning, TEXT("스킬 CancelAbility"));
}

// === 태그 기반 상태 관리 함수들 ===

bool UBaseGameplayAbility::HasAbilityState(FGameplayTag StateTag) const
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        return ASC->HasMatchingGameplayTag(StateTag);
    }
    return false;
}

void UBaseGameplayAbility::AddAbilityState(FGameplayTag StateTag)
{
    if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
    {
        ASC->SetAbilityState(StateTag, true);
    }
}

void UBaseGameplayAbility::RemoveAbilityState(FGameplayTag StateTag)
{
    if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
    {
        ASC->SetAbilityState(StateTag, false);
    }
}

// === 후속 입력 관리 ===

void UBaseGameplayAbility::RegisterFollowUpInput(FGameplayTag InputTag)
{
    ValidFollowUpInputs.AddUnique(InputTag);
    FollowUpInputTags.Add(InputTag);  // 하위 호환성
    
    if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
    {
        TSet<FGameplayTag> InputSet;
        for (const FGameplayTag& Tag : ValidFollowUpInputs)
        {
            InputSet.Add(Tag);
        }
        ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
    }
}

void UBaseGameplayAbility::RegisterFollowUpInputs(const TArray<FGameplayTag>& InputTags)
{
    ValidFollowUpInputs = InputTags;
    FollowUpInputTags.Empty();  // 하위 호환성
    for (const FGameplayTag& Tag : InputTags)
    {
        FollowUpInputTags.Add(Tag);
    }
    
    // 이미 활성화된 어빌리티라면 바로 등록
    if (IsActive())
    {
        if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
        {
            TSet<FGameplayTag> InputSet;
            for (const FGameplayTag& Tag : ValidFollowUpInputs)
            {
                InputSet.Add(Tag);
            }
            ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
        }
    }
}

void UBaseGameplayAbility::ClearFollowUpInputs()
{
    ValidFollowUpInputs.Empty();
    FollowUpInputTags.Empty();  // 하위 호환성
    
    if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
    {
        ASC->ClearFollowUpInputs();
    }
}

bool UBaseGameplayAbility::IsValidFollowUpInput(FGameplayTag InputTag) const
{
    return ValidFollowUpInputs.Contains(InputTag);
}

// === 개선된 입력 처리 ===

void UBaseGameplayAbility::HandleLeftClick(FGameplayEventData EventData)
{
    // MultiPhase 스킬 처리
    if (InputType == EAbilityInputType::MultiPhase && CurrentPhase == EAbilityPhase::Ready)
    {
        SetupPhase(EAbilityPhase::Executing);
        ExecutePhaseAction();
    }
    // 시퀀스 스킬 처리
    else if (InputType == EAbilityInputType::Sequence)
    {
        HandleSequenceAbility();
    }
    
    // 하위 호환성
    Active_Left_Click(EventData);
}

void UBaseGameplayAbility::HandleRightClick(FGameplayEventData EventData)
{
    // MultiPhase 또는 다른 스킬 유형에서 우클릭은 보통 취소로 처리
    if (InputType == EAbilityInputType::MultiPhase || 
        InputType == EAbilityInputType::Hold || 
        InputType == EAbilityInputType::Sequence)
    {
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
    }
    
    // 하위 호환성
    Active_Right_Click(EventData);
}

void UBaseGameplayAbility::HandleFollowUpInput(FGameplayTag InputTag, FGameplayEventData EventData)
{
    UE_LOG(LogTemp, Warning, TEXT("어빌리티 후속 입력 처리: %s"), *InputTag.ToString());
    
    if (!IsValidFollowUpInput(InputTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("유효하지 않은 후속 입력: %s"), *InputTag.ToString());
        return;
    }
    
    // 입력 태그에 따라 적절한 처리
    if (InputTag == FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")))
    {
        HandleLeftClick(EventData);
    }
    else if (InputTag == FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")))
    {
        HandleRightClick(EventData);
    }
    else
    {
        // 커스텀 후속 입력 처리 (하위 클래스에서 오버라이드 가능)
        UE_LOG(LogTemp, Warning, TEXT("커스텀 후속 입력 처리 필요: %s"), *InputTag.ToString());
    }
}

// === 어빌리티 타입별 처리 함수들 ===

void UBaseGameplayAbility::HandleInstantAbility()
{
    Active_General();
}

void UBaseGameplayAbility::HandleHoldAbility()
{
    if (!ValidFollowUpInputs.IsEmpty())
    {
        // 후속 입력 등록
        if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
        {
            TSet<FGameplayTag> InputSet;
            for (const FGameplayTag& Tag : ValidFollowUpInputs)
            {
                InputSet.Add(Tag);
            }
            ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
        }
        
        SetupPhase(EAbilityPhase::Ready);
        PlayReadyAnimation();
    }
    else
    {
        Active_General();
    }
}

void UBaseGameplayAbility::HandleToggleAbility()
{
    // 토글 스킬도 후속 입력이 있을 수 있음 (온/오프 전환)
    if (!ValidFollowUpInputs.IsEmpty())
    {
        if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
        {
            TSet<FGameplayTag> InputSet;
            for (const FGameplayTag& Tag : ValidFollowUpInputs)
            {
                InputSet.Add(Tag);
            }
            ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
        }
    }
    
    SetupPhase(EAbilityPhase::Ready);
    PlayReadyAnimation();
}

void UBaseGameplayAbility::HandleSequenceAbility()
{
    if (!ValidFollowUpInputs.IsEmpty())
    {
        // 후속 입력 등록
        if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
        {
            TSet<FGameplayTag> InputSet;
            for (const FGameplayTag& Tag : ValidFollowUpInputs)
            {
                InputSet.Add(Tag);
            }
            ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
        }
        
        SetupPhase(EAbilityPhase::Ready);
        PlayReadyAnimation();
    }
    else
    {
        Active_General();
    }
}

void UBaseGameplayAbility::HandleMultiPhaseAbility()
{
    if (!ValidFollowUpInputs.IsEmpty())
    {
        // 후속 입력 등록 (자동으로 WaitingFollowUp 상태 설정됨)
        if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
        {
            TSet<FGameplayTag> InputSet;
            for (const FGameplayTag& Tag : ValidFollowUpInputs)
            {
                InputSet.Add(Tag);
            }
            ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
        }
        
        SetupPhase(EAbilityPhase::Ready);
        PlayReadyAnimation();
    }
    else
    {
        Active_General();
    }
}

void UBaseGameplayAbility::HandleRepeatableAbility()
{
    SetupPhase(EAbilityPhase::Ready);
    CurrentRepeatCount = 0;
}

// === 단계 관리 ===

FGameplayTag UBaseGameplayAbility::GetCurrentPhaseTag() const
{
    switch (CurrentPhase)
    {
        case EAbilityPhase::Ready: return FValorantGameplayTags::Get().Phase_Ready;
        case EAbilityPhase::Aiming: return FValorantGameplayTags::Get().Phase_Aiming;
        case EAbilityPhase::Charging: return FValorantGameplayTags::Get().Phase_Charging;
        case EAbilityPhase::Executing: return FValorantGameplayTags::Get().Phase_Executing;
        case EAbilityPhase::Cooldown: return FValorantGameplayTags::Get().Phase_Cooldown;
        default: return FGameplayTag();
    }
}

void UBaseGameplayAbility::SetupPhase(EAbilityPhase NewPhase)
{
    EAbilityPhase OldPhase = CurrentPhase;
    CurrentPhase = NewPhase;
    
    // 이전 단계 이벤트
    if (OldPhase != EAbilityPhase::None)
    {
        OnPhaseExited(GetCurrentPhaseTag());
    }
    
    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    
    // 단계별 설정
    switch (NewPhase)
    {
        case EAbilityPhase::Ready:
            {
                if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Ready, false);
                PlayReadyEffects();
                break;
            }
        case EAbilityPhase::Aiming:
            {
                if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Aiming, true);
                PlayAimingEffects();
                break;
            }
        case EAbilityPhase::Charging:
            {
                if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Charging, true);
                PlayChargingEffects();
                HoldStartTime = GetWorld()->GetTimeSeconds();
                break;
            }
        case EAbilityPhase::Executing:
            {
                PlayExecuteEffects();
                break;
            }
        case EAbilityPhase::Cooldown:
            {
                PlayCooldownEffects();
                
                UAbilityTask_WaitDelay* CooldownTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.5f);
                CooldownTask->OnFinish.AddDynamic(this, &UBaseGameplayAbility::OnPhaseComplete);
                CooldownTask->ReadyForActivation();
                break;
            }
    }
    
    // 새 단계 이벤트
    OnPhaseEntered(GetCurrentPhaseTag());
    
    // 어빌리티 단계 변경 이벤트
    FGameplayEventData EventData;
    EventData.EventTag = FValorantGameplayTags::Get().Event_Ability_PhaseChanged;
    BroadcastAbilityEvent(FValorantGameplayTags::Get().Event_Ability_PhaseChanged, EventData);
    
    SetupPhaseTimeout();
}

void UBaseGameplayAbility::SetupPhaseByTag(FGameplayTag PhaseTag)
{
    if (PhaseTag == FValorantGameplayTags::Get().Phase_Ready)
        SetupPhase(EAbilityPhase::Ready);
    else if (PhaseTag == FValorantGameplayTags::Get().Phase_Aiming)
        SetupPhase(EAbilityPhase::Aiming);
    else if (PhaseTag == FValorantGameplayTags::Get().Phase_Charging)
        SetupPhase(EAbilityPhase::Charging);
    else if (PhaseTag == FValorantGameplayTags::Get().Phase_Executing)
        SetupPhase(EAbilityPhase::Executing);
    else if (PhaseTag == FValorantGameplayTags::Get().Phase_Cooldown)
        SetupPhase(EAbilityPhase::Cooldown);
}

void UBaseGameplayAbility::AdvanceToNextPhase()
{
    int32 NextPhaseIndex = static_cast<int32>(CurrentPhase) + 1;
    
    if (NextPhaseIndex > static_cast<int32>(EAbilityPhase::Cooldown))
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }
    
    EAbilityPhase NextPhase = static_cast<EAbilityPhase>(NextPhaseIndex);
    SetupPhase(NextPhase);
}

// === 초기화 및 정리 ===

void UBaseGameplayAbility::InitializeAbility()
{
    // 초기화 로직
}

void UBaseGameplayAbility::CleanupAbility()
{
    ClearAgentSkill(&CachedActorInfo);
    
    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (ASC)
    {
        // 모든 어빌리티 상태 정리
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Executing, false);
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Ready, false);
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_WaitingFollowUp, false);
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Charging, false);
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Aiming, false);
        
        ASC->ClearFollowUpInputs();
    }
}

// === 유틸리티 함수들 ===

void UBaseGameplayAbility::BroadcastAbilityEvent(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->HandleGameplayEvent(EventTag, &EventData);
    }
}

bool UBaseGameplayAbility::ConsumeAbilityStack(const APlayerController* PlayerController)
{
    AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
    if (PS == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 감소 실패: PlayerState가 NULL입니다."));
        return false;
    }

    int32 CurrentStack = PS->GetAbilityStack(m_AbilityID);
    if (CurrentStack > 0)
    {
        PS->ReduceAbilityStack(m_AbilityID);
        UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 감소됨. 남은 스택: %d"), m_AbilityID, CurrentStack - 1);
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 없습니다."), m_AbilityID);
    return false;
}

int32 UBaseGameplayAbility::GetAbilityStack(const APlayerController* PlayerController) const
{
    AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
    if (PS == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 확인 실패: PlayerState가 NULL입니다."));
        return 0;
    }

    return PS->GetAbilityStack(m_AbilityID);
}

bool UBaseGameplayAbility::SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo)
{
    if (ProjectileClass == nullptr)
    {
        return false;
    }

    AController* OwnerController = ActorInfo.PlayerController.Get();
    if (!OwnerController)
    {
        if (ActorInfo.OwnerActor.IsValid())
        {
            APawn* OwnerPawn = Cast<APawn>(ActorInfo.OwnerActor.Get());
            if (OwnerPawn)
            {
                OwnerController = OwnerPawn->GetController();
            }
        }

        if (!OwnerController)
        {
            return false;
        }
    }

    APlayerController* PlayerController = Cast<APlayerController>(OwnerController);
    if (!PlayerController)
    {
        return false;
    }

    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FVector CameraForward = CameraRotation.Vector();
    FVector SpawnLocation = CameraLocation;
    FRotator SpawnRotation = CameraRotation;
    
    FTransform SpawnTransform;
    SpawnTransform.SetLocation(SpawnLocation);
    SpawnTransform.SetRotation(SpawnRotation.Quaternion());
    SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.Instigator = Cast<APawn>(ActorInfo.OwnerActor.Get());
    SpawnParams.Owner = ActorInfo.OwnerActor.Get();
    
    AActor* SpawnedProjectile = GetWorld()->SpawnActor<AActor>(
        ProjectileClass,
        SpawnTransform,
        SpawnParams
    );

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    return (SpawnedProjectile != nullptr);
}

void UBaseGameplayAbility::OnPhaseTimeout()
{
    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UBaseGameplayAbility::OnPhaseComplete()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBaseGameplayAbility::OnAbilityComplete()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBaseGameplayAbility::PlayReadyAnimation()
{
    if (Ready3pMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, 
            "ReadyAnimation", 
            Ready3pMontage, 
            1.0f
        );
        
        if (Task)
        {
            Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnReadyAnimationCompleted);
            Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnReadyAnimationCompleted);
            Task->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnReadyAnimationCompleted);
            Task->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnReadyAnimationCompleted);
            Task->ReadyForActivation();
        }
        if (Ready1pMontage)
        {
            Play1pAnimation(Ready1pMontage);
        }
    }
    else
    {
        OnReadyAnimationCompleted();
    }
}

void UBaseGameplayAbility::Play1pAnimation(UAnimMontage* AnimMontage)
{
    if (auto* ps = Cast<AAgentPlayerState>(CachedActorInfo.OwnerActor))
    {
        auto* agent = Cast<ABaseAgent>(ps->GetPawn());
        if (agent)
        {
            agent->PlayFirstPersonMontage(AnimMontage);
        }
    }
}

void UBaseGameplayAbility::OnReadyAnimationCompleted()
{
    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (ASC)
    {
        ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Ready, true);
        NET_LOG(LogTemp, Warning, TEXT("스킬 준비 완료 - 후속 입력 대기"));
    }
}

void UBaseGameplayAbility::SetupPhaseTimeout()
{
    if (PhaseTimeoutDuration > 0.0f)
    {
        UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(
            this, PhaseTimeoutDuration);
        TimeoutTask->OnFinish.AddDynamic(this, &UBaseGameplayAbility::OnPhaseTimeout);
        TimeoutTask->ReadyForActivation();
    }
}

// === 기존 함수들 (하위 호환성) ===

void UBaseGameplayAbility::Active_General()
{
    // 하위 클래스에서 구현
}

void UBaseGameplayAbility::ClearAgentSkill(const FGameplayAbilityActorInfo* ActorInfo)
{
    if (auto* ps = Cast<AAgentPlayerState>(ActorInfo->OwnerActor))
    {
        auto* asc = Cast<UAgentAbilitySystemComponent>(ps->GetAbilitySystemComponent());
        auto* agent = Cast<ABaseAgent>(ps->GetPawn());
        
        if (asc == nullptr || agent == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("BaseGameplayAbility, ASC || Agent Null"));
            return;
        }

        asc->SetSkillClear(true);
        SetInputContext(false);
    }
}

void UBaseGameplayAbility::SetInputContext(bool bToAbilityContext)
{
    if (auto* ps = Cast<AAgentPlayerState>(CachedActorInfo.OwnerActor))
    {
        auto* agent = Cast<ABaseAgent>(ps->GetPawn());
        if (agent)
        {
            if (bToAbilityContext)
            {
                agent->SwitchEquipment(EInteractorType::Ability);
            }
            else
            {
                agent->SwitchEquipment(agent->GetPrevEquipmentType());
            }
        }
    }
}