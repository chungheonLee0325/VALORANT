#include "BaseGameplayAbility.h"
#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerState.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
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
    
    // 활성화 타입에 따른 시작 상태 결정
    switch (ActivationType)
    {
    case EAbilityActivationType::Instant:
        TransitionToState(EAbilityState::Executing);
        break;
        
    case EAbilityActivationType::WithPrepare:
        TransitionToState(EAbilityState::Preparing);
        break;
    }
}

void UBaseGameplayAbility::TransitionToState(EAbilityState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }
    
    EAbilityState OldState = CurrentState;
    
    // 이전 상태 종료
    ExitCurrentState();
    
    // 상태 변경
    CurrentState = NewState;
    
    // 태그 업데이트
    UpdateAbilityTags(NewState);
    
    // 새 상태 진입
    switch (NewState)
    {
    case EAbilityState::Preparing:
        EnterState_Preparing();
        break;
        
    case EAbilityState::Waiting:
        EnterState_Waiting();
        break;
        
    case EAbilityState::Executing:
        EnterState_Executing();
        break;
        
    case EAbilityState::Completed:
    case EAbilityState::Cancelled:
        CleanupAbility();
        break;
    }
    
    // 상태 변경 알림 (서버에서 클라이언트로)
    if (HasAuthority(&CurrentActivationInfo))
    {
        NotifyStateChanged(NewState);
    }
    
    // 델리게이트 브로드캐스트
    OnStateChanged.Broadcast(ConvertStateToTag(NewState));
    
    NET_LOG(LogTemp, Display, TEXT("어빌리티 상태 전환: %s -> %s"),
            *UEnum::GetValueAsString(OldState),
            *UEnum::GetValueAsString(NewState));
}

void UBaseGameplayAbility::EnterState_Preparing()
{
    PrepareAbility();
    
    // 준비 VFX / SFX 재생
    if (HasAuthority(&CurrentActivationInfo))
    {
        PlayCommonEffects(PrepareEffect, nullptr, FVector(0));
    }
    if (IsLocallyControlled())
    {
        if (PrepareSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), PrepareSound);
        }
    }
    
    // 준비 애니메이션 재생
    if (PrepareMontage_1P || PrepareMontage_3P)
    {
        PlayMontages(PrepareMontage_1P, nullptr);
        
        // 몽타주 완료 대기
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, PrepareMontage_3P, 1.0f);
        
        if (Task)
        {
            Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
            Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
            Task->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
            Task->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
            Task->ReadyForActivation();
        }
    }
    else
    {
        // 애니메이션이 없으면 바로 대기 상태로
        TransitionToState(EAbilityState::Waiting);
    }
}

void UBaseGameplayAbility::EnterState_Waiting()
{
    FGameplayTag inputTag;
    for (const FGameplayTag& Tag : AbilityTags)
    {
        if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Input.Skill"))))
        {
            inputTag = Tag;
            break;
        }
    }
    
    OnWaitAbility.Broadcast(inputTag, FollowUpInputType);
    
    WaitAbility();
    
    if (HasAuthority(&CurrentActivationInfo))
    {
        // 대기 VFX / SFX 재생
        PlayCommonEffects(WaitEffect, nullptr, FVector(0));
    }
    if (IsLocallyControlled())
    {
        if (WaitSound)
        {
            UGameplayStatics::PlaySound2D(GetWorld(), WaitSound);
        }
    }
    
    // 대기 애니메이션 재생 (루프)
    if (WaitingMontage_1P || WaitingMontage_3P)
    {
        PlayMontages(WaitingMontage_1P, WaitingMontage_3P, false);
    }
    
    // 타임아웃 설정
    if (FollowUpTime > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(WaitingTimeoutHandle, this,
                                              &UBaseGameplayAbility::OnWaitingTimeout, FollowUpTime, false);
    }
}

void UBaseGameplayAbility::EnterState_Executing()
{
    // 서버에서만 스택 소모
    if (HasAuthority(&CurrentActivationInfo))
    {
        ReduceAbilityStack();
        // 실행 VFX / SFX 재생
        PlayCommonEffects(ExecuteEffect, ExecuteSound, FVector(0));
        // 실행 결과를 클라이언트에 알림
        NotifyAbilityExecuted(true);
        // ASC에 실행 알림
        if (CachedASC)
        {
            CachedASC->MulticastRPC_OnAbilityExecuted(GetAssetTags().First(), true);
        }
    }

    // 후속 입력 UI 숨김 처리
    OnFollowUpInput.Broadcast();
    
    ExecuteAbility();

    // 입력 타입에 따라 몽타주 재생
    UAnimMontage* Montage1P = nullptr;
    UAnimMontage* Montage3P = nullptr;
    if (LastExecuteInputType == EFollowUpInputType::LeftClick)
    {
        Montage1P = ExecuteLeftMouseButtonMontage_1P ? ExecuteLeftMouseButtonMontage_1P : ExecuteMontage_1P;
        Montage3P = ExecuteLeftMouseButtonMontage_3P ? ExecuteLeftMouseButtonMontage_3P : ExecuteMontage_3P;
    }
    else if (LastExecuteInputType == EFollowUpInputType::RightClick)
    {
        Montage1P = ExecuteRightMouseButtonMontage_1P ? ExecuteRightMouseButtonMontage_1P : ExecuteMontage_1P;
        Montage3P = ExecuteRightMouseButtonMontage_3P ? ExecuteRightMouseButtonMontage_3P : ExecuteMontage_3P;
    }
    else
    {
        Montage1P = ExecuteMontage_1P;
        Montage3P = ExecuteMontage_3P;
    }

    LastExecuteInputType = EFollowUpInputType::None;

    if (Montage1P || Montage3P)
    {
        PlayMontages(Montage1P, nullptr);
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, Montage3P, 1.0f);
        if (Task)
        {
            Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
            Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
            Task->ReadyForActivation();
        }
    }
    else
    {
        // 애니메이션이 없으면 바로 완료
        TransitionToState(EAbilityState::Completed);
    }
}

void UBaseGameplayAbility::ExitCurrentState()
{
    switch (CurrentState)
    {
    case EAbilityState::Waiting:
        // 타이머 정리
        GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);
        break;
    }
}

void UBaseGameplayAbility::HandleFollowUpInput(FGameplayTag InputTag)
{
    if (CurrentState != EAbilityState::Waiting)
    {
        return;
    }
    
    if (!IsValidFollowUpInput(InputTag))
    {
        return;
    }
    
    bool bShouldExecute = false;
    
    // 입력 타입에 따라 처리
    if (InputTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick)
    {
        bShouldExecute = OnLeftClickInput();
        if (bShouldExecute)
        {
            LastExecuteInputType = EFollowUpInputType::LeftClick;
        }
    }
    else if (InputTag == FValorantGameplayTags::Get().InputTag_Default_RightClick)
    {
        bShouldExecute = OnRightClickInput();
        if (bShouldExecute)
        {
            LastExecuteInputType = EFollowUpInputType::RightClick;
        }
    }
    
    if (bShouldExecute)
    {
        TransitionToState(EAbilityState::Executing);
    }
}

void UBaseGameplayAbility::CleanupAbility()
{
    // 모든 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);
    GetWorld()->GetTimerManager().ClearTimer(CleanupDelayHandle);
    
    // 몽타주 정지
    StopAllMontages();
    
    // 약간의 딜레이 후 최종 정리 (무기 전환 등)
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
    bool bWasCancelled = (CurrentState == EAbilityState::Cancelled);
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    bool bReplicateEndAbility, bool bWasCancelled)
{
    // 중복 호출 방지
    if (CurrentState == EAbilityState::None)
    {
        return;
    }
    
    // 타이머 정리
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    
    CurrentState = EAbilityState::None;
    
    // 모든 태그 정리
    UpdateAbilityTags(EAbilityState::None);

    // 캐시된 정보 정리
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
    if (CurrentState == EAbilityState::None || 
        CurrentState == EAbilityState::Cancelled ||
        CurrentState == EAbilityState::Completed)
    {
        return;
    }
    
    // 실행 중 취소 가능 여부 확인
    if (CurrentState == EAbilityState::Executing && !bAllowCancelDuringExecution)
    {
        return;
    }
    
    TransitionToState(EAbilityState::Cancelled);
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

// 유틸리티 함수들
void UBaseGameplayAbility::UpdateAbilityTags(EAbilityState NewState)
{
    if (!CachedASC)
    {
        return;
    }
    
    // 모든 상태 태그 제거
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
    CachedASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
    
    // 새 상태에 따른 태그 추가
    switch (NewState)
    {
    case EAbilityState::Preparing:
        CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
        //CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
        break;
        
    case EAbilityState::Waiting:
        CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
        //CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
        break;
        
    case EAbilityState::Executing:
        CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
        CachedASC->AddLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
        break;
    }
}

bool UBaseGameplayAbility::IsValidFollowUpInput(FGameplayTag InputTag) const
{
    switch (FollowUpInputType)
    {
    case EFollowUpInputType::LeftClick:
        return InputTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick;
        
    case EFollowUpInputType::RightClick:
        return InputTag == FValorantGameplayTags::Get().InputTag_Default_RightClick;
        
    case EFollowUpInputType::LeftOrRight:
        return InputTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick ||
               InputTag == FValorantGameplayTags::Get().InputTag_Default_RightClick;
        
    default:
        return false;
    }
}

FGameplayTag UBaseGameplayAbility::ConvertStateToTag(EAbilityState State) const
{
    switch (State)
    {
    case EAbilityState::Preparing:
        return FValorantGameplayTags::Get().State_Ability_Preparing;
    case EAbilityState::Waiting:
        return FValorantGameplayTags::Get().State_Ability_Waiting;
    case EAbilityState::Executing:
        return FValorantGameplayTags::Get().State_Ability_Executing;
    default:
        return FGameplayTag();
    }
}

// 몽타주 관련
void UBaseGameplayAbility::OnMontageCompleted()
{
    switch (CurrentState)
    {
    case EAbilityState::Preparing:
        TransitionToState(EAbilityState::Waiting);
        break;
        
    case EAbilityState::Executing:
        TransitionToState(EAbilityState::Completed);
        break;
    }
}

void UBaseGameplayAbility::OnMontageBlendOut()
{
    // 몽타주가 중단된 경우
    if (IsActive())
    {
        OnMontageCompleted();
    }
}

void UBaseGameplayAbility::OnWaitingTimeout()
{
    NET_LOG(LogTemp, Warning, TEXT("후속 입력 타임아웃"));
    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

// 네트워크 동기화 - Actor를 통한 간접 호출
void UBaseGameplayAbility::NotifyStateChanged(EAbilityState NewState)
{
    // ASC를 통한 상태 동기화
    if (CachedASC)
    {
        FGameplayTag StateTag = ConvertStateToTag(NewState);
        CachedASC->MulticastRPC_OnAbilityExecuted(GetAssetTags().First(), true);
    }
}

void UBaseGameplayAbility::NotifyAbilityExecuted(bool bSuccess)
{
    // ASC를 통한 실행 결과 동기화
    if (CachedASC)
    {
        CachedASC->MulticastRPC_OnAbilityExecuted(GetAssetTags().First(), bSuccess);
    }
    
    // 추가로 GameplayCue를 통한 시각적 피드백
    if (bSuccess)
    {
        // FGameplayCueParameters CueParams;
        // CueParams.SourceObject = GetAvatarActorFromActorInfo();
        //
        // // 어빌리티별 Cue 태그 (예: GameplayCue.Ability.SlowOrb.Execute)
        // FGameplayTag CueTag = FGameplayTag::RequestGameplayTag(
        //     FName(FString::Printf(TEXT("GameplayCue.Ability.%s.Execute"), 
        //         *GetAssetTags().First().GetTagName().ToString())));
        //
        // CachedASC->ExecuteGameplayCue(CueTag, CueParams);
    }
}

void UBaseGameplayAbility::PrepareAbility()
{
}

void UBaseGameplayAbility::WaitAbility() {}
void UBaseGameplayAbility::ExecuteAbility() {}
bool UBaseGameplayAbility::OnLeftClickInput() { return false; }
bool UBaseGameplayAbility::OnRightClickInput() { return false; }

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
    
    // 1인칭 몽타주
    if (Montage1P)
    {
        Agent->Client_PlayFirstPersonMontage(Montage1P);
    }
    
    // 3인칭 몽타주
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
    
    // 카메라 위치에서 스폰
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