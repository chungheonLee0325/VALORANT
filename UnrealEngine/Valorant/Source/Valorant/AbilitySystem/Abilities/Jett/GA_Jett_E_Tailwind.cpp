// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Jett_E_Tailwind.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" // If using montages
#include "Abilities/Tasks/AbilityTask_WaitDelay.h" // For simple delays
#include "Player/AgentPlayerState.h"
#include "Valorant/AbilitySystem/Attributes/Jett/JettAttributeSet.h" // Needed to check charges

UGA_Jett_E_Tailwind::UGA_Jett_E_Tailwind()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Jett.Tailwind")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Jett.Tailwind.Active")));
	// Tag added while ability is active

	// Define tags this ability blocks
	// BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability"))); // Example: Block all other abilities?

	// Define tags this ability requires to activate (e.g., player must be grounded)
	// ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Grounded")));

	// Define tags this ability is blocked by (e.g., player is stunned)
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Stunned")));
	// Add the cooldown tag here
	// ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Cooldown.Tailwind")));

	// Ability Instancing Policy (Instanced per execution is common for abilities with state/timers)
	//InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UGA_Jett_E_Tailwind::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayTagContainer* SourceTags,
                                             const FGameplayTagContainer* TargetTags,
                                             OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Check for cooldown tag (redundant with ActivationBlockedTags but good practice)
	if (ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag))
	{
		return false;
	}

	// Check for sufficient charges - ToDo : 현재 서버에만 갯수 동기화중, 변경전까진 서버에서만 검사
	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (Avatar->GetLocalRole() == ROLE_Authority)
	{
		ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
		if (0 == GetAbilityStack(Character, m_AbilityID))
		{
			return false;
		}
	}

	return true;
}

void UGA_Jett_E_Tailwind::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!Character || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // Cancel if invalid
		return;
	}
	// Commit the ability cost etc. Kills the ability if `cost cannot be met.
	// This should ideally happen *before* applying the cost GE if the cost GE has a check itself,
	// or after if the cost is just a number reduction. Let's place it here for simplicity.

	// 최종 확인
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to commit ability Tailwind"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // Cancel if commit fails
		return;
	}

	// Cost 소모
	ReduceAbilityStack(Character, m_AbilityID);
	UE_LOG(LogTemp, Log, TEXT("Tailwind Activated"));

	// --- 1. Perform the Dash ---
	// Get dash direction (movement input or forward)
	FVector DashVel = Character->GetLastMovementInputVector();
	if (DashVel.IsNearlyZero())
	{
		DashVel = Character->GetActorForwardVector();
	}
	DashVel.Z = 0; // Usually dash horizontally
	DashVel.Normalize();

	// Calculate launch velocity
	// Option A: Instant impulse using LaunchCharacter
	// float LaunchSpeed = DashDistance / DashDuration; // Simplistic calculation
	// Character->LaunchCharacter(DashVel * LaunchSpeed, true, true);

	// Option B: Use an AbilityTask or custom movement logic for smoother control
	// This gives better control over the movement curve and duration.
	// For simplicity here, we use LaunchCharacter and a delay timer.
	// A more robust solution would involve an AbilityTask or setting a custom movement mode.

	// Calculate velocity needed to cover DashDistance in DashDuration
	// Note: LaunchCharacter applies an impulse, so exact distance/duration is tricky without friction/drag consideration.
	// For a more precise dash, consider overriding PhysWalking or using Root Motion / Custom Movement Component state.
	const float LaunchSpeed = 5000.0f; // Adjust this based on testing
	Character->LaunchCharacter(DashVel * LaunchSpeed, true, true);

	// --- 2. Wait for Dash Completion & End Ability ---
	UAbilityTask_WaitDelay* WaitDelay = UAbilityTask_WaitDelay::WaitDelay(this, DashDuration);
	if (WaitDelay)
	{
		WaitDelay->OnFinish.AddDynamic(this, &UGA_Jett_E_Tailwind::OnDashFinished);
		WaitDelay->ReadyForActivation();
	}
	else
	{
		// Fallback or handle error
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}

	// You might also play a montage here using UAbilityTask_PlayMontageAndWait
}

void UGA_Jett_E_Tailwind::OnDashFinished()
{
	// This is called when the WaitDelay task finishes
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Jett_E_Tailwind::FinishDash()
{
	// This is called if using a simple FTimerHandle
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

int32 UGA_Jett_E_Tailwind::GetAbilityStack(const ACharacter* Character, int32 AbilityID)
{
	AAgentPlayerState* PS = Cast<AAgentPlayerState>(Character->GetPlayerState());
	if (!PS)
	{
		return 0;
	}
	return PS->GetAbilityStack(AbilityID);
}

void UGA_Jett_E_Tailwind::ReduceAbilityStack(const ACharacter* Character, int32 AbilityID)
{
	AAgentPlayerState* PS = Cast<AAgentPlayerState>(Character->GetPlayerState());
	if (!PS)
	{
		return;
	}
	PS->ReduceAbilityStack(AbilityID);
}


void UGA_Jett_E_Tailwind::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                     bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// Cleanup code here if needed (e.g., stopping montages)

	// IMPORTANT: Remove the Cooldown Tag GE *if* it has a duration matching the dash.
	// However, for Jett, the "cooldown" is just having 0 charges. The tag GE might be infinite duration
	// and only removed when a charge is granted. If the tag *is* meant to be temporary (e.g., prevent spamming during the dash itself),
	// remove it here. Let's assume the tag is infinite and linked to having 0 charges, so we don't remove it here.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && CooldownTagGE)
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(
			CooldownTagGE, ActorInfo->AbilitySystemComponent.Get(), 1);
	}
	UE_LOG(LogTemp, Log, TEXT("Tailwind Ended. Cancelled: %s"), bWasCancelled ? TEXT("True") : TEXT("False"));
}
