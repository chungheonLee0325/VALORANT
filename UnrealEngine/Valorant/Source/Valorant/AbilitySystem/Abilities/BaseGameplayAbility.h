// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "BaseGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetAbilityID(int32 AbilityID);

	UFUNCTION(BlueprintCallable)
	void SetFollowUpInputTag(FGameplayTag inputTag) { FollowUpInputTags.Add(inputTag); }
	UFUNCTION(BlueprintCallable)
	void ClearFollowUpInputTag() { FollowUpInputTags.Empty(); }
	
	// UFUNCTION(BlueprintCallable)
	// void SetCurrentFollowUpInput(const FGameplayTag inputTag) { CurrentFollowUpInputTag = inputTag; }
	
	// 어빌리티 스택 감소 메서드
	UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
	bool ConsumeAbilityStack();
	
	// 어빌리티 스택 확인 메서드 
	UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
	int32 GetAbilityStack() const;

protected:
	UPROPERTY(EditAnywhere)
	int32 m_AbilityID = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TSet<FGameplayTag> FollowUpInputTags;

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	
	virtual void Active_General();
	
	UFUNCTION()
	virtual void Active_Left_Click(FGameplayEventData data);
	UFUNCTION()
	virtual void Active_Right_Click(FGameplayEventData data);
};
