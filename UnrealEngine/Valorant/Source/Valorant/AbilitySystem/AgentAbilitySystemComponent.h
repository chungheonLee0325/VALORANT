// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
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

protected:
	virtual void BeginPlay() override;

	//AttributeSet
	void InitializeAttribute();

	//Ability
	void RegisterAgentAbilities();
	void ClearAgentAbilities();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	FAgentData* m_AgentData;
	

	UPROPERTY()
	UGameplayAbility* Ability_C = nullptr;
	UPROPERTY()
	UGameplayAbility* Ability_Q = nullptr;
	UPROPERTY()
	UGameplayAbility* Ability_E = nullptr;
	UPROPERTY()
	UGameplayAbility* Ability_X = nullptr;
};
