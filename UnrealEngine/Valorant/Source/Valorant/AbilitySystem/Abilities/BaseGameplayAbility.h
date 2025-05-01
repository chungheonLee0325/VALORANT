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
	
	UFUNCTION(BlueprintCallable)
	void SetCurrentFollowUpInput(const FGameplayTag inputTag) { CurrentFollowUpInputTag = inputTag; }
	

protected:
	UPROPERTY(EditAnywhere)
	int32 m_AbilityID = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TSet<FGameplayTag> FollowUpInputTags;
	
	UPROPERTY()
	FGameplayTag CurrentFollowUpInputTag;

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	
	virtual void Active_General();
	virtual void Active_Left_Click();
	virtual void Active_Right_Click();
	virtual void Active_C_Click();
	virtual void Active_E_Click();
	virtual void Active_Q_Click();
	virtual void Active_X_Click();
};
