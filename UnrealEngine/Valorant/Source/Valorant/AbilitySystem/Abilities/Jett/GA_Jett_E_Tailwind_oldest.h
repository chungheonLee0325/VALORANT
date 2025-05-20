// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Valorant/AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "GA_Jett_E_Tailwind_oldest.generated.h"

class UGameplayEffect;
class UAbilityTask_WaitGameplayEvent; // Forward declaration
/**
 * 
 */
UCLASS()
class VALORANT_API UGA_Jett_E_Tailwind_oldest : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Jett_E_Tailwind_oldest();

	/** Returns true if this ability can be activated right now. Has no side effects */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr,
	                                const FGameplayTagContainer* TargetTags = nullptr,
	                                OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Actually activate ability, do not call this directly */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	/** Destroys instanced-per-execution abilities. Instance per actor are stable */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	// Gameplay Effect to apply the cost (reduce TailwindCharges)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GameplayEffects")
	TSubclassOf<UGameplayEffect> CostGE;

	// Gameplay Effect to apply a tag preventing re-activation during dash/no charges
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GameplayEffects")
	TSubclassOf<UGameplayEffect> CooldownTagGE;

	// Tag applied by CooldownTagGE
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Tags")
	FGameplayTag CooldownTag;

	// Distance of the dash
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tailwind")
	float DashDistance = 1000.0f;

	// Duration of the dash
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tailwind")
	float DashDuration = 0.2f; // Adjust as needed


	// Example using an ability task to handle the movement and timer
	UFUNCTION()
	void OnDashFinished();

	// Example using a simple timer if AbilityTask is not used
	FTimerHandle DashTimerHandle;
	void FinishDash();

protected:
	static int32 GetAbilityStack(const ACharacter* Character, int32 AbilityID);
	static void ReduceAbilityStack(const ACharacter* Character, int32 AbilityID);
};
