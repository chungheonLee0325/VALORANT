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
#include "ValorantObject/BaseInteractor.h"

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
	if (auto* agent = Cast<ABaseAgent>(ActorInfo->AvatarActor))
	{
		if (agent == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("BaseGameplayAbility, Agent Null"));
			return;
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
	if (InputType == EAbilityInputType::Hold &&
		(CurrentAbilityState == FValorantGameplayTags::Get().State_Ability_Ready ||
			CurrentAbilityState == FValorantGameplayTags::Get().State_Ability_Preparing))
	{
		// Ready 상태에서만 Charging으로 전환
		if (CurrentAbilityState == FValorantGameplayTags::Get().State_Ability_Ready)
		{
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Charging);
			HoldStartTime = GetWorld()->GetTimeSeconds();
		}
	}

	// 반복 입력 스킬 처리
	if (InputType == EAbilityInputType::Repeatable)
	{
		if (CurrentRepeatCount < MaxRepeatCount)
		{
			ExecuteStateAction();
			CurrentRepeatCount++;

			if (CurrentRepeatCount >= MaxRepeatCount)
			{
				TransitionToState(FValorantGameplayTags::Get().State_Ability_Ended);
			}
		}
	}

	// 토글 스킬 처리
	if (InputType == EAbilityInputType::Toggle)
	{
		if (CurrentAbilityState == FValorantGameplayTags::Get().State_Ability_Ready)
		{
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
		}
		else if (CurrentAbilityState == FValorantGameplayTags::Get().State_Ability_Executing)
		{
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Ended);
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
	if (InputType == EAbilityInputType::Hold && CurrentAbilityState == FValorantGameplayTags::Get().
		State_Ability_Charging)
	{
		float HoldTime = GetWorld()->GetTimeSeconds() - HoldStartTime;

		if (HoldTime >= MinHoldDuration)
		{
			HoldTime = FMath::Min(HoldTime, MaxHoldDuration);
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
			ExecuteStateAction(HoldTime);
		}
		else
		{
			CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		}
	}

	NET_LOG(LogTemp, Warning, TEXT("스킬 InputReleased"));
}

bool UBaseGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         FGameplayTagContainer* OptionalRelevantTags)
{
	ConsumeAbilityStack(ActorInfo->PlayerController.Get());
	return Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
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

void UBaseGameplayAbility::SetAbilityState(FGameplayTag StateTag, bool bApply)
{
	if (UAgentAbilitySystemComponent* ASC = Cast<
		UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->SetAbilityState(StateTag, bApply);
	}
}

void UBaseGameplayAbility::RemoveAbilityState(FGameplayTag StateTag)
{
	if (UAgentAbilitySystemComponent* ASC = Cast<
		UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->SetAbilityState(StateTag, false);
	}
}

// === 후속 입력 관리 ===

void UBaseGameplayAbility::RegisterFollowUpInput(FGameplayTag InputTag)
{
	ValidFollowUpInputs.AddUnique(InputTag);

	if (UAgentAbilitySystemComponent* ASC = Cast<
		UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
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

	// 이미 활성화된 어빌리티라면 바로 등록
	if (IsActive())
	{
		if (UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(
			GetAbilitySystemComponentFromActorInfo()))
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

	if (UAgentAbilitySystemComponent* ASC = Cast<
		UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->ClearFollowUpInputs();
	}
}

bool UBaseGameplayAbility::IsValidFollowUpInput(FGameplayTag InputTag) const
{
	return ValidFollowUpInputs.Contains(InputTag);
}

// === 입력 처리 ===

void UBaseGameplayAbility::HandleLeftClick(FGameplayEventData EventData)
{
	// MultiPhase 스킬 처리
	if (InputType == EAbilityInputType::MultiPhase && CurrentAbilityState == FValorantGameplayTags::Get().
		State_Ability_Ready)
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
		ExecuteStateAction();
	}
	// 시퀀스 스킬 처리
	else if (InputType == EAbilityInputType::Sequence)
	{
		HandleSequenceAbility();
	}
}

void UBaseGameplayAbility::HandleRightClick(FGameplayEventData EventData)
{
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
	TransitionToState(FValorantGameplayTags::Get().State_Ability_Preparing);
	// TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
	// ExecuteStateAction();
}

void UBaseGameplayAbility::HandleHoldAbility()
{
	if (!ValidFollowUpInputs.IsEmpty())
	{
		// 애니메이션이 있는 경우
		if (Ready3pMontage || Ready1pMontage)
		{
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Preparing);
			PlayReadyAnimation();
		}
		else
		{
			// 애니메이션이 없으면 바로 Ready 상태로 전환하고 후속 입력 등록
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Ready);
			OnPreparingAnimationCompleted();
		}
	}
	else
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
		ExecuteStateAction();
	}
}

void UBaseGameplayAbility::HandleToggleAbility()
{
	if (!ValidFollowUpInputs.IsEmpty())
	{
		// 애니메이션이 있는 경우
		if (Ready3pMontage || Ready1pMontage)
		{
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Preparing);
			PlayReadyAnimation();
		}
		else
		{
			// 애니메이션이 없으면 바로 Ready 상태로 전환하고 후속 입력 등록
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Ready);
			OnPreparingAnimationCompleted();
		}
	}
	else
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Ready);
		PlayReadyAnimation();
	}
}

void UBaseGameplayAbility::HandleSequenceAbility()
{
	if (!ValidFollowUpInputs.IsEmpty())
	{
		// 애니메이션이 있는 경우
		if (Ready3pMontage || Ready1pMontage)
		{
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Preparing);
			PlayReadyAnimation();
		}
		else
		{
			// 애니메이션이 없으면 바로 Ready 상태로 전환하고 후속 입력 등록
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Ready);
			OnPreparingAnimationCompleted();
		}
	}
	else
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
		ExecuteStateAction();
	}
}

void UBaseGameplayAbility::HandleMultiPhaseAbility()
{
	if (!ValidFollowUpInputs.IsEmpty())
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Preparing);
		PlayReadyAnimation();
	}
	else
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
		ExecuteStateAction();
	}
}

void UBaseGameplayAbility::HandleRepeatableAbility()
{
	TransitionToState(FValorantGameplayTags::Get().State_Ability_Ready);
	CurrentRepeatCount = 0;
}

void UBaseGameplayAbility::HandlePreparingState()
{
	// 애니메이션 재생
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
			Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->ReadyForActivation();
		}
		if (Ready1pMontage)
		{
			Play1pAnimation(Ready1pMontage);
		}
	}
	else
	{
		// 애니메이션이 없으면 바로 완료 처리
		OnPreparingAnimationCompleted();
	}
}

void UBaseGameplayAbility::HandleReadyState()
{
	UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	// 후속 입력이 있는 경우에만 등록
	if (!ValidFollowUpInputs.IsEmpty())
	{
		TSet<FGameplayTag> InputSet;
		for (const FGameplayTag& Tag : ValidFollowUpInputs)
		{
			InputSet.Add(Tag);
		}
		ASC->RegisterFollowUpInputs(InputSet, GetAssetTags().First());
	}
	else
	{
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
	}

	NET_LOG(LogTemp, Warning, TEXT("스킬 준비 완료 - 후속 입력 대기"));
}

void UBaseGameplayAbility::HandleExecutingState()
{
	// 애니메이션 재생
	if (Executing3pMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			"ExecutingAnimation",
			Executing3pMontage,
			1.0f
		);

		if (Task)
		{
			Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnExecutingAnimationCompleted);
			Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnExecutingAnimationCompleted);
			Task->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnExecutingAnimationCompleted);
			Task->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnExecutingAnimationCompleted);
			Task->ReadyForActivation();
		}
		if (Executing1pMontage)
		{
			Play1pAnimation(Executing1pMontage);
		}
	}
	else
	{
		// 애니메이션이 없으면 바로 완료 처리
		OnExecutingAnimationCompleted();
	}
}

// === 상태 관리 ===

void UBaseGameplayAbility::TransitionToState(FGameplayTag NewState)
{
	FGameplayTag OldState = CurrentAbilityState;
	CurrentAbilityState = NewState;

	// 이전 상태 이벤트
	if (OldState.IsValid())
	{
		OnStateExited(OldState);
	}

	UAgentAbilitySystemComponent* ASC = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());

	// 상태별 설정
	if (NewState == FValorantGameplayTags::Get().State_Ability_Preparing)
	{
		if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Preparing, true);
		HandlePreparingState();
		// 애니메이션 재생 중이므로 아직 후속 입력 등록하지 않음
	}
	else if (NewState == FValorantGameplayTags::Get().State_Ability_Ready)
	{
		if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Ready, true);
		HandleReadyState();
		PlayReadyEffects();
	}
	else if (NewState == FValorantGameplayTags::Get().State_Ability_Aiming)
	{
		if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Aiming, true);
		PlayAimingEffects();
	}
	else if (NewState == FValorantGameplayTags::Get().State_Ability_Charging)
	{
		if (ASC) ASC->SetAbilityState(FValorantGameplayTags::Get().State_Ability_Charging, true);
		PlayChargingEffects();
		HoldStartTime = GetWorld()->GetTimeSeconds();
	}
	else if (NewState == FValorantGameplayTags::Get().State_Ability_Executing)
	{
		HandleExecutingState();
		PlayExecuteEffects();
	}
	else if (NewState == FValorantGameplayTags::Get().State_Ability_Ended)
	{
		OnAbilityComplete();
	}

	// 새 상태 이벤트
	OnStateEntered(NewState);

	// 어빌리티 상태 변경 이벤트
	FGameplayEventData EventData;
	EventData.EventTag = FValorantGameplayTags::Get().Event_Ability_StateChanged;
	BroadcastAbilityEvent(FValorantGameplayTags::Get().Event_Ability_StateChanged, EventData);

	SetupStateTimeout();
}

void UBaseGameplayAbility::AdvanceToNextState()
{
	// 상태 진행 로직 (하위 클래스에서 구현)
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// === 초기화 및 정리 ===

void UBaseGameplayAbility::InitializeAbility()
{
	// 초기화 로직
}

void UBaseGameplayAbility::CleanupAbility()
{
	if (auto* ps = Cast<AAgentPlayerState>(CachedActorInfo.OwnerActor))
	{
		auto* asc = Cast<UAgentAbilitySystemComponent>(ps->GetAbilitySystemComponent());
		auto* agent = Cast<ABaseAgent>(ps->GetPawn());

		if (asc == nullptr || agent == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("BaseGameplayAbility, ASC || Agent Null"));
			return;
		}

		// 모든 어빌리티 상태 정리 (Preparing 상태 포함)
		asc->CleanupAbilityState();
		SetInputContext(false);
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

	//EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	return (SpawnedProjectile != nullptr);
}

void UBaseGameplayAbility::OnStateTimeout()
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
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
			Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnPreparingAnimationCompleted);
			Task->ReadyForActivation();
		}
		if (Ready1pMontage)
		{
			Play1pAnimation(Ready1pMontage);
		}
	}
	else
	{
		// 애니메이션이 없으면 바로 완료 처리
		OnPreparingAnimationCompleted();
	}
}

void UBaseGameplayAbility::Play1pAnimation(UAnimMontage* AnimMontage)
{
	if (auto* agent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor))
	{
		agent->PlayFirstPersonMontage(AnimMontage);
	}
}

void UBaseGameplayAbility::OnPreparingAnimationCompleted()
{
	// 현재 상태가 Preparing인지 확인 (에러 처리)
	if (CurrentAbilityState != FValorantGameplayTags::Get().State_Ability_Preparing)
	{
		UE_LOG(LogTemp, Warning, TEXT("애니메이션 완료되었으나 Preparing 상태가 아님: %s"),
		       *CurrentAbilityState.ToString());
		return;
	}

	// 애니메이션 완료 후에 비로소 Ready 상태로 전환
	TransitionToState(FValorantGameplayTags::Get().State_Ability_Ready);
}

void UBaseGameplayAbility::OnExecutingAnimationCompleted()
{
	TransitionToState(FValorantGameplayTags::Get().State_Ability_Ended);
}

void UBaseGameplayAbility::SetupStateTimeout()
{
	if (StateTimeoutDuration > 0.0f)
	{
		UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(
			this, StateTimeoutDuration);
		TimeoutTask->OnFinish.AddDynamic(this, &UBaseGameplayAbility::OnStateTimeout);
		TimeoutTask->ReadyForActivation();
	}
}

void UBaseGameplayAbility::SetInputContext(bool bToAbilityContext)
{
	if (auto* agent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor))
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
