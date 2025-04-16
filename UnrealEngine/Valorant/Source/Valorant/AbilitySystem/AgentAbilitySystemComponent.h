// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "InputTriggers.h"
#include "ValorantGameplayTags.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "AgentAbilitySystemComponent.generated.h"


UCLASS()
class VALORANT_API UAgentAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UAgentAbilitySystemComponent();
	
	void InitializeAgentData(FAgentData* agentData);
	
	FAgentData* GetAgentData() const { return m_AgentData; }

	UFUNCTION(BlueprintCallable)
	void SkillCallByTag(const FGameplayTag& inputTag);

private:
	FAgentData* m_AgentData;

	UPROPERTY()
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> SkillHandleMap;
	
	TSet<FGameplayTag> InputSlots = {
		FValorantGameplayTags::Get().InputTag_Ability_Q,
		FValorantGameplayTags::Get().InputTag_Ability_E,
		FValorantGameplayTags::Get().InputTag_Ability_C,
		FValorantGameplayTags::Get().InputTag_Ability_X
	};

	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ActiveAbilitiesSlot;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	//AttributeSet
	void InitializeAttribute();

	//Ability
	void RegisterAgentAbilities();
	void ClearAgentAbilities();
	void ClearAgentAbility(FGameplayAbilitySpec* spec);
	void GiveAgentAbility(TSubclassOf<UGameplayAbility> abilityClass, int32 level);
};
