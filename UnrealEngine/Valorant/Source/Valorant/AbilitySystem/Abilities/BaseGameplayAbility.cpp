#include "BaseGameplayAbility.h"
#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerState.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AgentAbility/BaseProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    bReplicateInputDirectly = true;
    
    // 차단 태그
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Suppressed")));
}

void UBaseGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);
    
    // 델리게이트 연결
    if (ABaseAgent* Agent = Cast<ABaseAgent>(ActorInfo->AvatarActor.Get()))
    {
        OnWaitAbility.RemoveDynamic(Agent, &ABaseAgent::OnAbilityPrepare);
        OnWaitAbility.AddDynamic(Agent, &ABaseAgent::OnAbilityPrepare);
        OnFollowUpInput.RemoveDynamic(Agent, &ABaseAgent::OnAbilityFollowupInput);
        OnFollowUpInput.AddDynamic(Agent, &ABaseAgent::OnAbilityFollowupInput);
        OnEndAbility.RemoveDynamic(Agent, &ABaseAgent::OnEndAbility);
        OnEndAbility.AddDynamic(Agent, &ABaseAgent::OnEndAbility);
    }
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    // 캐시 정보 저장
    CachedActorInfo = *ActorInfo;
    CachedASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    
    // 현재 장비 상태 저장
    if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
    {
        PreviousEquipmentState = Agent->GetInteractorState();
        
        // 어빌리티 모드로 전환
        if (HasAuthority(&ActivationInfo))
        {
            Agent->SwitchEquipment(EInteractorType::Ability);
        }
    }
    
    // 활성화 타입에 따른 시작
    switch (ActivationType)
    {
    case EAbilityActivationType::Instant:
        StartExecutePhase();
        break;
        
    case EAbilityActivationType::WithPrepare:
        StartPreparePhase();
        break;
    }
}

void UBaseGameplayAbility::StartPreparePhase()
{
    // 상태 설정
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Preparing);
    
    // 준비 동작 실행
    PrepareAbility();
    
    // 효과 재생
    if (HasAuthority(&CurrentActivationInfo))
    {
        PlayCommonEffects(PrepareEffect, nullptr, FVector(0));
    }
    if (IsLocallyControlled() && PrepareSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), PrepareSound);
    }
    
    // 애니메이션 재생
    if (PrepareMontage_1P || PrepareMontage_3P)
    {
        PlayMontages(PrepareMontage_1P, nullptr);
        
        // 몽타주 태스크 생성
        ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, PrepareMontage_3P, 1.0f);
        
        if (ActiveMontageTask)
        {
            ActiveMontageTask->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnPrepareMontageCompleted);
            ActiveMontageTask->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnPrepareMontageBlendOut);
            ActiveMontageTask->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnPrepareMontageBlendOut);
            ActiveMontageTask->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnPrepareMontageBlendOut);
            ActiveMontageTask->ReadyForActivation();
        }
    }
    else
    {
        // 애니메이션이 없으면 바로 대기 단계로
        StartWaitingPhase();
    }
}

void UBaseGameplayAbility::StartWaitingPhase()
{
    // 상태 설정
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Waiting);
    
    // UI 알림을 위한 입력 태그 찾기
    FGameplayTag inputTag;
    for (const FGameplayTag& Tag : GetAssetTags())
    {
        if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Input.Skill"))))
        {
            inputTag = Tag;
            break;
        }
    }
    
    OnWaitAbility.Broadcast(inputTag, FollowUpInputType);
    WaitAbility();
    
    // 효과 재생
    if (HasAuthority(&CurrentActivationInfo))
    {
        PlayCommonEffects(WaitEffect, nullptr, FVector(0));
    }
    if (IsLocallyControlled() && WaitSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), WaitSound);
    }
    
    // 대기 애니메이션 재생
    if (WaitingMontage_1P || WaitingMontage_3P)
    {
        PlayMontages(WaitingMontage_1P, WaitingMontage_3P, false);
    }

    // 후속 입력 대기 태스크를 생성합니다.
    if (FollowUpInputType != EFollowUpInputType::None)
    {
        // 람다 함수를 사용하여 WaitGameplayEvent 태스크 생성 및 활성화를 캡슐화합니다.
        // 코드 중복을 줄이고 올바른 함수 시그니처 사용을 명확히 합니다.
        auto CreateAndWaitTask = [this](FGameplayTag EventTag)
        {
            // 지정된 EventTag를 기다리는 태스크를 생성합니다.
            UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EventTag);
            if (WaitEventTask)
            {
                WaitEventTask->EventReceived.AddDynamic(this, &UBaseGameplayAbility::OnFollowUpEventReceived);
            
                // 태스크를 활성화 준비 상태로 만듭니다.
                WaitEventTask->ReadyForActivation();
            }
        };

        // FollowUpInputType에 따라 적절한 입력 이벤트를 대기합니다.
        switch (FollowUpInputType)
        {
        case EFollowUpInputType::LeftClick:
            CreateAndWaitTask(FValorantGameplayTags::Get().InputTag_Default_LeftClick);
            break;

        case EFollowUpInputType::RightClick:
            CreateAndWaitTask(FValorantGameplayTags::Get().InputTag_Default_RightClick);
            break;

        case EFollowUpInputType::LeftOrRight:
            // 좌클릭과 우클릭 이벤트에 대해 각각 별도의 태스크를 생성하여 둘 중 하나라도 들어오면 처리되도록 합니다.
            CreateAndWaitTask(FValorantGameplayTags::Get().InputTag_Default_LeftClick);
            CreateAndWaitTask(FValorantGameplayTags::Get().InputTag_Default_RightClick);
            break;

        default:
            // 처리되지 않은 EFollowUpInputType 값에 대한 경고 로그입니다.
            UE_LOG(LogTemp, Warning, TEXT("UBaseGameplayAbility::ActivateAbility - Unhandled EFollowUpInputType in ability %s"), *GetName());
            // 의도치 않은 타입이므로 함수를 종료합니다.
            EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
            return;
        }
    }
    
    // 타임아웃 설정
    if (FollowUpTime > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(WaitingTimeoutHandle, this,
                                              &UBaseGameplayAbility::OnWaitingTimeout, FollowUpTime, false);
    }
}

void UBaseGameplayAbility::StartExecutePhase(EFollowUpInputType InputType)
{
    // 대기 관련 정리
    GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);
    if (WaitEventTask)
    {
        WaitEventTask->EndTask();
        WaitEventTask = nullptr;
    }
    
    // 상태 설정
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Executing);
    
    // 스택 소모 및 실행
    if (HasAuthority(&CurrentActivationInfo))
    {
        ReduceAbilityStack();
        PlayCommonEffects(ExecuteEffect, ExecuteSound, FVector(0));
        
        if (CachedASC)
        {
            CachedASC->MulticastRPC_OnAbilityExecuted(GetAssetTags().First(), true);
        }
    }

    // UI 숨김 처리
    OnFollowUpInput.Broadcast();
    
    LastInputType = InputType;
    ExecuteAbility();

    // 입력 타입에 따라 몽타주 선택
    UAnimMontage* Montage1P = nullptr;
    UAnimMontage* Montage3P = nullptr;
    
    if (InputType == EFollowUpInputType::LeftClick)
    {
        Montage1P = ExecuteLeftMouseButtonMontage_1P ? ExecuteLeftMouseButtonMontage_1P : ExecuteMontage_1P;
        Montage3P = ExecuteLeftMouseButtonMontage_3P ? ExecuteLeftMouseButtonMontage_3P : ExecuteMontage_3P;
    }
    else if (InputType == EFollowUpInputType::RightClick)
    {
        Montage1P = ExecuteRightMouseButtonMontage_1P ? ExecuteRightMouseButtonMontage_1P : ExecuteMontage_1P;
        Montage3P = ExecuteRightMouseButtonMontage_3P ? ExecuteRightMouseButtonMontage_3P : ExecuteMontage_3P;
    }
    else
    {
        Montage1P = ExecuteMontage_1P;
        Montage3P = ExecuteMontage_3P;
    }

    if (Montage1P || Montage3P)
    {
        PlayMontages(Montage1P, nullptr);
        
        ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, Montage3P, 1.0f);
            
        if (ActiveMontageTask)
        {
            ActiveMontageTask->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnExecuteMontageCompleted);
            ActiveMontageTask->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnExecuteMontageBlendOut);
            ActiveMontageTask->ReadyForActivation();
        }
    }
    else
    {
        CompleteAbility();
    }
}

void UBaseGameplayAbility::CompleteAbility()
{
    // 모든 태스크 정리
    if (ActiveMontageTask)
    {
        ActiveMontageTask->EndTask();
        ActiveMontageTask = nullptr;
    }
    if (WaitEventTask)
    {
        WaitEventTask->EndTask();
        WaitEventTask = nullptr;
    }
    
    // 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);
    
    // 몽타주 정지
    StopAllMontages();
    
    // 상태 정리
    ClearAllAbilityStates();
    
    // 약간의 딜레이 후 최종 정리
    GetWorld()->GetTimerManager().SetTimer(CleanupDelayHandle, this,
                                          &UBaseGameplayAbility::PerformFinalCleanup, 0.1f, false);
}

void UBaseGameplayAbility::PerformFinalCleanup()
{
    // 무기 복귀
    if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
    {
        if (PreviousEquipmentState != EInteractorType::None &&
            Agent->GetInteractorState() == EInteractorType::Ability)
        {
            Agent->SwitchEquipment(PreviousEquipmentState);
        }
    }
    
    // 어빌리티 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    bool bReplicateEndAbility, bool bWasCancelled)
{
    // 타이머 정리
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    
    // 태스크 정리
    if (ActiveMontageTask)
    {
        ActiveMontageTask->EndTask();
        ActiveMontageTask = nullptr;
    }
    if (WaitEventTask)
    {
        WaitEventTask->EndTask();
        WaitEventTask = nullptr;
    }
    
    // 상태 정리
    ClearAllAbilityStates();
    
    // 캐시 정리
    CachedActorInfo = FGameplayAbilityActorInfo();
    CachedASC = nullptr;
    SpawnedProjectile = nullptr;
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       bool bReplicateCancelAbility)
{
    // 실행 중 취소 가능 여부 확인
    if (IsInExecutingState() && !bAllowCancelDuringExecution)
    {
        return;
    }
    
    CompleteAbility();
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
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

    // 에이전트가 죽었는지 확인
    if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
    {
        if (Agent->IsDead())
        {
            NET_LOG(LogTemp, Warning, TEXT("죽은 에이전트는 어빌리티를 사용할 수 없습니다"));
            return false;
        }
    }
    
    // 스택 확인
    return GetAbilityStack() > 0;
}

// 태스크 콜백들
void UBaseGameplayAbility::OnPrepareMontageCompleted()
{
    StartWaitingPhase();
}

void UBaseGameplayAbility::OnPrepareMontageBlendOut()
{
    if (IsActive())
    {
        OnPrepareMontageCompleted();
    }
}

void UBaseGameplayAbility::OnExecuteMontageCompleted()
{
    CompleteAbility();
}

void UBaseGameplayAbility::OnExecuteMontageBlendOut()
{
    if (IsActive())
    {
        OnExecuteMontageCompleted();
    }
}

void UBaseGameplayAbility::OnWaitingTimeout()
{
    NET_LOG(LogTemp, Warning, TEXT("후속 입력 타임아웃"));
    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UBaseGameplayAbility::OnFollowUpEventReceived(FGameplayEventData Payload)
{
    if (!CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting))
    {
        return;
    }
    
    FGameplayTag EventTag;
    EventTag = Payload.EventTag;
    NET_LOG(LogTemp, Warning, TEXT("후속 입력 이벤트 수신: %s"), *EventTag.ToString());
    
    // 입력 타입 결정
    EFollowUpInputType InputType = EFollowUpInputType::None;
    bool bShouldExecute = false;
    
    if (EventTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick)
    {
        InputType = EFollowUpInputType::LeftClick;
        bShouldExecute = OnLeftClickInput();
    }
    else if (EventTag == FValorantGameplayTags::Get().InputTag_Default_RightClick)
    {
        InputType = EFollowUpInputType::RightClick;
        bShouldExecute = OnRightClickInput();
    }
    
    if (bShouldExecute)
    {
        StartExecutePhase(InputType);
    }
}

// 상태 관리 헬퍼
void UBaseGameplayAbility::SetAbilityState(const FGameplayTag& StateTag)
{
    if (!CachedASC)
    {
        return;
    }
    
    // 이전 상태 태그 제거
    ClearAllAbilityStates();
    
    // 새 상태 태그 추가
    CachedASC->AddLooseGameplayTag(StateTag);
    
    // 무기 전환 차단 태그는 실행 중에만
    if (StateTag == FValorantGameplayTags::Get().State_Ability_Executing)
    {
        CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
    }
    
    // 상태 변경 알림
    OnStateChanged.Broadcast(StateTag);
}

void UBaseGameplayAbility::ClearAllAbilityStates()
{
    if (!CachedASC)
    {
        return;
    }
    
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
}

FGameplayTag UBaseGameplayAbility::GetCurrentStateTag() const
{
    if (!CachedASC)
    {
        return FGameplayTag();
    }
    
    if (CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing))
        return FValorantGameplayTags::Get().State_Ability_Preparing;
    if (CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting))
        return FValorantGameplayTags::Get().State_Ability_Waiting;
    if (CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing))
        return FValorantGameplayTags::Get().State_Ability_Executing;
    
    return FGameplayTag();
}

bool UBaseGameplayAbility::IsInPreparingState() const
{
    return CachedASC && CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
}

bool UBaseGameplayAbility::IsInWaitingState() const
{
    return CachedASC && CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
}

bool UBaseGameplayAbility::IsInExecutingState() const
{
    return CachedASC && CachedASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
}

// 유틸리티 함수들
void UBaseGameplayAbility::PlayMontages(UAnimMontage* Montage1P, UAnimMontage* Montage3P, bool bStopAllMontages)
{
    if (!HasAuthority(&CurrentActivationInfo))
    {
        return;
    }
    
    if (bStopAllMontages)
    {
        StopAllMontages();
    }
    
    ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
    if (!Agent)
    {
        return;
    }
    
    if (Montage1P)
    {
        Agent->Client_PlayFirstPersonMontage(Montage1P);
    }
    
    if (Montage3P)
    {
        Agent->NetMulti_PlayThirdPersonMontage(Montage3P);
    }
}

void UBaseGameplayAbility::StopAllMontages()
{
    ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
    if (!Agent || Agent->IsDead())
    {
        return;
    }
    
    Agent->StopThirdPersonMontage(0.05f);
    Agent->StopFirstPersonMontage(0.05f);
}

bool UBaseGameplayAbility::SpawnProjectile(FVector LocationOffset, FRotator RotationOffset)
{
    if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
    {
        return false;
    }
    
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        return false;
    }
    
    UCameraComponent* CameraComp = Character->FindComponentByClass<UCameraComponent>();
    FVector SpawnLocation;
    FRotator SpawnRotation;
    
    if (CameraComp)
    {
        SpawnLocation = CameraComp->GetComponentLocation() +
            CameraComp->GetForwardVector() * 30.0f +
            CameraComp->GetRightVector() * 15.0f +
            CameraComp->GetUpVector() * -10.0f +
            LocationOffset;
        
        SpawnRotation = CameraComp->GetComponentRotation() + RotationOffset;
    }
    else
    {
        FVector EyeLocation = Character->GetActorLocation() + FVector(0, 0, Character->BaseEyeHeight);
        SpawnLocation = EyeLocation + Character->GetActorForwardVector() * 50.0f + LocationOffset;
        SpawnRotation = Character->GetControlRotation() + RotationOffset;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;
    SpawnParams.Instigator = Character;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    SpawnedProjectile = GetWorld()->SpawnActor<ABaseProjectile>(
        ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );
    
    return SpawnedProjectile != nullptr;
}

void UBaseGameplayAbility::PlayCommonEffects(UNiagaraSystem* NiagaraEffect, USoundBase* SoundEffect, FVector Location)
{
    if (!HasAuthority(&CurrentActivationInfo))
    {
        return;
    }
    
    ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
    if (!Agent)
    {
        return;
    }
    
    if (Location.IsZero())
    {
        Location = Agent->GetActorLocation();
    }
    
    if (NiagaraEffect)
    {
        Agent->Multicast_PlayNiagaraEffectAtLocation(Location, NiagaraEffect);
    }
    
    if (SoundEffect)
    {
        Agent->Multicast_PlaySoundAtLocation(Location, SoundEffect);
    }
}

bool UBaseGameplayAbility::ReduceAbilityStack()
{
    // PreRound나 BuyPhase가 아닐 때만 스택 소모
    if (AMatchGameState* MatchGS = GetWorld()->GetGameState<AMatchGameState>())
    {
        ERoundSubState CurrentRoundState = MatchGS->GetRoundSubState();
        if (CurrentRoundState == ERoundSubState::RSS_PreRound || 
            CurrentRoundState == ERoundSubState::RSS_BuyPhase)
        {
            return false;
        }
    }
    
    if (const APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get()))
    {
        if (AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>())
        {
            return PS->ReduceAbilityStack(m_AbilityID) > 0;
        }
    }
    return false;
}

int32 UBaseGameplayAbility::GetAbilityStack() const
{
    if (const APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get()))
    {
        if (const AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>())
        {
            return PS->GetAbilityStack(m_AbilityID);
        }
    }
    return 0;
}