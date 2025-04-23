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
	void SetAbilityID(int32 AbilityID);

protected:
	UPROPERTY(EditAnywhere)
	int32 m_AbilityID = 0;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TSet<FGameplayTag> FollowUpInputTags;

	UPROPERTY()
	FGameplayTag CurrentFollowUpInputTag;

	//TODO: CanActivateAbility(): 스킬 실행 가능한지 체크, 쿨타임 블락태그 등
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	//TODO: ActiveAbility(): 스킬 가짜 실행, (필요하다면) ASC에 후속 입력 키 등록
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	//TODO: InputPressd(): 스킬 진짜 실행
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	
	//TODO: InputReleased()
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	
	//TODO: EndAbility(): ASC->ClearCurrentAbilityHandle 호출
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	//TODO: CancelAbility(): ASC->ClearCurrentAbilityHandle 호출
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;

	// Ability에서 ASC가져오는 법?
	// UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	// 직접 멤버 변수로 캐싱해도 되지만, 필요할 때마다 호출해도 퍼포먼스 이슈 없음
};
