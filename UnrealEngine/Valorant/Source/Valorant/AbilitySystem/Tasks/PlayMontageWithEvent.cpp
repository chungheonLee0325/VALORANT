#include "PlayMontageWithEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Player/Agent/BaseAgent.h"

UPlayMontageWithEvent::UPlayMontageWithEvent()
{
	Rate = 1.f;
	bStopWhenAbilityEnds = true;
	PlayAnimMontage = nullptr;
}

void UPlayMontageWithEvent::Activate()
{
	if (!IsValid(PlayAnimMontage))
	{
		return;
	}

	// GA 없으면 진행할 Task가 없으니 그대로 중단
	if (!IsValid(Ability))
	{
		return;
	}
	// ASC가 존재하지 않는 경우에도 더이상 GATask 추적에는 의미가 없음
	if (!AbilitySystemComponent.IsValid())
	{
		return;
	}
	// 비동기로 동작하는 AnimMontage가 정상적으로 동작했는지 확인하는 별도의 변수
	bool IsAlreadyPlayedMontage = false;
	// 현재 GA를 활성화한 Actor 정보
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();

	// 현재 Actor의 Animation Instance 정보 가져오기
	if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
	{
		// 이전 Montage는 강제 종료 시켜버린다.
		AbilitySystemComponent->CurrentMontageStop();
		// GameplayTag에 대해서 ChangedEvent가 발생할 때의 Delegate 정보 자체를 EventHandle에 저장
		EventHandle = AbilitySystemComponent->AddGameplayEventTagContainerDelegate(
			EventTags
			, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(
				this, &UPlayMontageWithEvent::OnGameplayEvent));
		// ASC에서 현재 GA가 활성화된 Actor(등록된 Actor가 아닌 것이 핵심) 대상으로 PlayMontage를 진행
		if (AbilitySystemComponent->PlayMontage(
			Ability, Ability->GetCurrentActivationInfo(), PlayAnimMontage, Rate
			, StartSection) > 0.f)
		{
			// GATask 에서 Delegate가 실행될 것이 없다면 중단
			if (ShouldBroadcastAbilityTaskDelegates() == false)
			{
				return;
			}
			// GA 캔슬에 대한 Delegate 등록
			CancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(
				this, &UPlayMontageWithEvent::OnAbilityCancelled);

			// Anim Montage의 Blend Out시에 대한 Delegate 등록
			BlendingOutDelegate.BindUObject(
				this, &UPlayMontageWithEvent::OnMontageBlendingOut);
			AnimInstance->Montage_SetBlendingOutDelegate(
				BlendingOutDelegate, PlayAnimMontage);

			// Anim Montage의 End에 대한 Delegate 등록
			MontageEndedDelegate.BindUObject(
				this, &UPlayMontageWithEvent::OnMontageEnded);
			AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, PlayAnimMontage);

			ABaseAgent* Character = Cast<ABaseAgent>(GetAvatarActor());
			// 현재 캐릭터가 서버 역할을 담당하고, 캐릭터가 자발적 Proxy 상태이고, GA의 Net 실행 정책이 Local Predicted 상태인 경우
			if (Character && (Character->GetLocalRole() == ROLE_Authority || (
				Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->
				GetNetExecutionPolicy() ==
				EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
			{
				// Animation의 RMS 값 설정 (중요하지는 않다)
				Character->SetAnimRootMotionTranslationScale(
					AnimRootMotionTranslationScale);
			}

			IsAlreadyPlayedMontage = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning
						, TEXT( "PlayMontageAndWaitForEvent call to PlayMontage failed!" ));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning
					, TEXT(
						"PlayMontageAndWaitForEvent called on invalid AbilitySystemComponent"
					));
	}

	if (!IsAlreadyPlayedMontage)
	{
		UE_LOG(LogTemp, Warning
					, TEXT(
						"PlayMontageAndWaitForEvent called in Ability %s failed to play montage %s; Task Instance Name %s."
					), *Ability->GetName(), *GetNameSafe(PlayAnimMontage)
					, *InstanceName.ToString());
		// Anim Montage가 다 실행되지도 않았는데 아직 실행해야할 GATask Delegate 정보가 남아있으면 Cancel 관련 Delegate를 전부 실행한다.
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), EventData);
		}
	}

	SetWaitingOnAvatar();
}


UPlayMontageWithEvent* UPlayMontageWithEvent::InitialEvent(
	UGameplayAbility* OwningAbility, FName TaskInstanceName
	, UAnimMontage* MontageToPlay, FGameplayTagContainer EventTags
	, FGameplayEventData EventData, float Rate
	, FName StartSection, bool bStopWhenAbilityEnds
	, float AnimRootMotionTranslationScale)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	UPlayMontageWithEvent* NewEvent = NewAbilityTask<UPlayMontageWithEvent>(
		OwningAbility, TaskInstanceName);

	NewEvent->PlayAnimMontage = MontageToPlay;
	NewEvent->Rate = Rate;
	NewEvent->EventData = EventData;

	return NewEvent;
}

void UPlayMontageWithEvent::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		EventReceived.Broadcast(EventTag, TempData);
	}
}

void UPlayMontageWithEvent::OnAbilityCancelled()
{
	if (StopPlayingMontage())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), EventData);
		}
	}
}

void UPlayMontageWithEvent::OnMontageBlendingOut(
	UAnimMontage* Montage, bool bInterrupted)
{
	if (Ability && Ability->GetCurrentMontage() == PlayAnimMontage)
	{
		if (Montage == PlayAnimMontage)
		{
			AbilitySystemComponent->ClearAnimatingAbility(Ability);

			// Reset AnimRootMotionTranslationScale
			ABaseAgent* Character = Cast<ABaseAgent>(GetAvatarActor());
			if (Character && (Character->GetLocalRole() == ROLE_Authority || (
				Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->
				GetNetExecutionPolicy() ==
				EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
			{
				Character->SetAnimRootMotionTranslationScale(1.f);
			}
		}
	}

	if (bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), EventData);
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast(FGameplayTag(), EventData);
		}
	}
}

void UPlayMontageWithEvent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(FGameplayTag(), EventData);
		}
	}

	EndTask();
}


bool UPlayMontageWithEvent::StopPlayingMontage()
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return false;
	}

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return false;
	}

	if (AbilitySystemComponent.IsValid() && Ability)
	{
		if (AbilitySystemComponent->GetAnimatingAbility() == Ability &&
			AbilitySystemComponent->GetCurrentMontage() == PlayAnimMontage)
		{
			if (FAnimMontageInstance* MontageInstance = AnimInstance->
				GetActiveInstanceForMontage(PlayAnimMontage))
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			AbilitySystemComponent->CurrentMontageStop();
			return true;
		}
	}

	return false;
}

FString UPlayMontageWithEvent::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			PlayingMontage = AnimInstance->Montage_IsActive(PlayAnimMontage)
												? PlayAnimMontage
												: AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(
		TEXT(
			"PlayMontageAndWaitForEvent. PlayAnimMontage: %s  (Currently Playing): %s")
		, *GetNameSafe(PlayAnimMontage), *GetNameSafe(PlayingMontage));
}

void UPlayMontageWithEvent::ExternalCancel()
{
	check(AbilitySystemComponent.IsValid());

	OnAbilityCancelled();

	Super::ExternalCancel();
}

void UPlayMontageWithEvent::OnDestroy(bool AbilityEnded)
{
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(CancelledHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage();
		}
	}

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(
			EventTags, EventHandle);
	}

	Super::OnDestroy(AbilityEnded);
}
