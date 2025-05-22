// Fill out your copyright notice in the Description page of Project Settings.

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
    // 기본 활성화 조건 확인
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 어빌리티 ID가 유효하고 스택 소비가 필요한 경우
    if (m_AbilityID > 0)
    {
        // 플레이어 스테이트 가져오기
        AAgentPlayerState* PS = Cast<AAgentPlayerState>(ActorInfo->PlayerController->PlayerState);
        if (PS == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("어빌리티 활성화 확인 실패: PlayerState가 NULL입니다."));
            return false;
        }

        // 현재 스택 확인
        int32 CurrentStack = GetAbilityStack(ActorInfo->PlayerController.Get());

        // 스택이 없으면 활성화 불가
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

    m_ActorInfo = *ActorInfo;

    SetInputContext(true);

    UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (asc)
    {
        // 입력 타입에 따라 다른 초기화 로직
        switch (InputType)
        {
            case EAbilityInputType::Instant:
                // 즉시 실행 로직
                Active_General();
                break;
                
            case EAbilityInputType::Hold:
            case EAbilityInputType::Toggle:
            case EAbilityInputType::Sequence:
            case EAbilityInputType::MultiPhase:
            case EAbilityInputType::Repeatable:
                // 후속 입력이 필요한 스킬의 경우
                if (FollowUpInputTags.IsEmpty() == false)
                {
                    asc->SetSkillReady(false);
                    asc->SetSkillClear(false);
                    asc->ResisterFollowUpInput(FollowUpInputTags);
                    
                    // 준비 단계 시작
                    SetupPhase(EAbilityPhase::Ready);

                    // 준비 애니메이션 재생 - 애니메이션 완료 이벤트를 설정하여 Ready 상태로 만듦
                    PlayReadyAnimation();
                }
                else
                {
                    Active_General();
                }
                break;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GA, asc가 AgentAbilitySystemComponent를 상속받지 않았어요."));
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
            
            // 최대 반복 도달시 마무리
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
            // 홀드 시간 제한
            HoldTime = FMath::Min(HoldTime, MaxHoldDuration);
            
            // 실행 단계로 전환
            SetupPhase(EAbilityPhase::Executing);
            ExecutePhaseAction(HoldTime);
        }
        else
        {
            // 최소 홀드 시간 미달시 취소
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

    ClearAgentSkill(ActorInfo);
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    NET_LOG(LogTemp, Warning, TEXT("스킬 EndAbility"));
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         bool bReplicateCancelAbility)
{
    ClearAgentSkill(ActorInfo);
    
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
    NET_LOG(LogTemp, Warning, TEXT("스킬 CancelAbility"));
}

void UBaseGameplayAbility::Active_General()
{
    // 하위 클래스에서 구현
}

void UBaseGameplayAbility::Active_Left_Click(FGameplayEventData data)
{
    // MultiPhase 스킬 처리 (피닉스 핫핸즈 등)
    if (InputType == EAbilityInputType::MultiPhase && CurrentPhase == EAbilityPhase::Ready)
    {
        SetupPhase(EAbilityPhase::Executing);
        ExecutePhaseAction();
    }
    // 시퀀스 스킬 처리
    else if (InputType == EAbilityInputType::Sequence)
    {
        // 시퀀스 진행 로직 (구체적인 시퀀스는 하위 클래스에서 구현)
    }
}

void UBaseGameplayAbility::Active_Right_Click(FGameplayEventData data)
{
    // MultiPhase 또는 다른 스킬 유형에서 우클릭은 보통 취소로 처리
    if (InputType == EAbilityInputType::MultiPhase || 
        InputType == EAbilityInputType::Hold || 
        InputType == EAbilityInputType::Sequence)
    {
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
    }
    // 시퀀스 스킬에서는 시퀀스의 일부로 처리할 수도 있음
    else if (InputType == EAbilityInputType::Sequence)
    {
        // 시퀀스 진행 로직 (구체적인 시퀀스는 하위 클래스에서 구현)
    }
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
        
        // 입력 컨텍스트 복원
        //agent->SwitchEquipment(EEquipmentType::MainWeapon);
        SetInputContext(false);
    }
}

bool UBaseGameplayAbility::SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo)
{
    if (ProjectileClass == nullptr)
    {
        return false;
    }

    // Get the owning controller 
    AController* OwnerController = ActorInfo.PlayerController.Get();
    if (!OwnerController)
    {
        // Try to get controller from pawn if player controller is not available
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

    // Get the player controller for screen-to-world projection
    APlayerController* PlayerController = Cast<APlayerController>(OwnerController);
    if (!PlayerController)
    {
        return false;
    }

    // 카메라 위치와 회전 구하기
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // 카메라 전방 벡터 가져오기 (화면 중앙이 바라보는 방향)
    FVector CameraForward = CameraRotation.Vector();

    // 발사 위치 계산 (카메라 위치)
    FVector SpawnLocation = CameraLocation;
    
    // 발사 회전 (카메라 회전과 동일)
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

    // 투사체 발사 후 어빌리티 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    return (SpawnedProjectile != nullptr);
}

void UBaseGameplayAbility::SetAbilityID(int32 AbilityID)
{
    m_AbilityID = AbilityID;
}

bool UBaseGameplayAbility::ConsumeAbilityStack(const APlayerController* PlayerController)
{
    // 플레이어 스테이트 가져오기
    AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
    if (PS == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 감소 실패: PlayerState가 NULL입니다."));
        return false;
    }

    // 현재 스택 가져오기
    int32 CurrentStack = PS->GetAbilityStack(m_AbilityID);

    // 스택이 있으면 감소
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
    // 플레이어 스테이트 가져오기
    AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
    if (PS == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 확인 실패: PlayerState가 NULL입니다."));
        return 0;
    }

    // 현재 스택 반환
    return PS->GetAbilityStack(m_AbilityID);
}

void UBaseGameplayAbility::SetupPhase(EAbilityPhase NewPhase)
{
    // 이전 단계 정리
    UAbilityTask_WaitDelay* TimeoutTask = nullptr;
    if (TimeoutTask)
    {
        TimeoutTask->EndTask();
        TimeoutTask = nullptr;
    }
    
    // 새 단계 설정
    CurrentPhase = NewPhase;

    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    
    // 단계별 설정
    switch (NewPhase)
    {
        case EAbilityPhase::Ready:
            {
                PlayReadyEffects();
                break;
            }
        case EAbilityPhase::Aiming:
            {
                PlayAimingEffects();
                break;
            }
        case EAbilityPhase::Charging:
            {
                PlayChargingEffects();
                // 홀드 시간 기록
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
            
                // 일정 시간 후 어빌리티 완전히 종료
                UAbilityTask_WaitDelay* CooldownTask = UAbilityTask_WaitDelay::WaitDelay(
                    this, 0.5f);
                CooldownTask->OnFinish.AddDynamic(this, &UBaseGameplayAbility::OnPhaseComplete);
                CooldownTask->ReadyForActivation();
                break;
            }
    }
    
    // 필요시 타임아웃 설정
    SetupPhaseTimeout();
}

void UBaseGameplayAbility::AdvanceToNextPhase()
{
    // 다음 단계 계산
    int32 NextPhaseIndex = static_cast<int32>(CurrentPhase) + 1;
    
    // 최대 단계 체크
    if (NextPhaseIndex > static_cast<int32>(EAbilityPhase::Cooldown))
    {
        // 모든 단계 완료, 스킬 종료
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }
    
    // 다음 단계로 설정
    EAbilityPhase NextPhase = static_cast<EAbilityPhase>(NextPhaseIndex);
    SetupPhase(NextPhase);
}

void UBaseGameplayAbility::SetupPhaseTimeout()
{
    // 타임아웃이 설정된 경우에만 처리
    if (PhaseTimeoutDuration > 0.0f)
    {
        UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(
            this, PhaseTimeoutDuration);
        TimeoutTask->OnFinish.AddDynamic(this, &UBaseGameplayAbility::OnPhaseTimeout);
        TimeoutTask->ReadyForActivation();
    }
}

void UBaseGameplayAbility::OnPhaseTimeout()
{
    // 타임아웃 시 처리 (보통 스킬 취소)
    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UBaseGameplayAbility::OnPhaseComplete()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBaseGameplayAbility::PlayReadyAnimation()
{
    // 애니메이션 몽타주 재생
    if (ReadyMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, 
            "ReadyAnimation", 
            ReadyMontage, 
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
    }
    else
    {
        // 애니메이션이 없는 경우 바로 준비 완료 처리
        OnReadyAnimationCompleted();
    }
}

void UBaseGameplayAbility::OnReadyAnimationCompleted()
{
    // 준비 애니메이션 완료 후 스킬 준비 상태로 변경
    UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (ASC)
    {
        ASC->SetSkillReady(true);
        NET_LOG(LogTemp, Warning, TEXT("스킬 준비 완료 - 후속 입력 대기"));
    }
}

void UBaseGameplayAbility::SetInputContext(bool bToAbilityContext)
{
    // Agent에서 입력 컨텍스트 설정
    if (auto* ps = Cast<AAgentPlayerState>(CurrentActorInfo->OwnerActor))
    {
        auto* agent = Cast<ABaseAgent>(ps->GetPawn());
        if (agent)
        {
            // 어빌리티 모드로 전환 또는 무기 모드로 복원
            if (bToAbilityContext)
            {
                agent->SwitchEquipment(EEquipmentType::Ability); // 스킬 모드
            }
            else
            {
                agent->SwitchEquipment(agent->GetPrevEquipmentType()); // 무기 모드
            }
        }
    }
}
