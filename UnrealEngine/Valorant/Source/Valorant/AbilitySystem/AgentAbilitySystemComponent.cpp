// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"

#include "Attributes/BaseAttributeSet.h"


UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAgentAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAgentAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAgentAbilitySystemComponent::InitializeAgentData(FAgentData* agentData)
{
	m_AgentData = agentData;
	
	InitializeAttribute();
	RegisterAgentAbilities();
}

void UAgentAbilitySystemComponent::InitializeAttribute()
{
	SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), m_AgentData->BaseHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), m_AgentData->MaxHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(),m_AgentData->BaseArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), m_AgentData->MaxArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), m_AgentData->Speed);
}

void UAgentAbilitySystemComponent::RegisterAgentAbilities()
{
	GiveAbility(m_AgentData->Ability_C);
	GiveAbility(m_AgentData->Ability_E);
	GiveAbility(m_AgentData->Ability_Q);
	GiveAbility(m_AgentData->Ability_X);
}

void UAgentAbilitySystemComponent::ClearAgentAbilities()
{
	FGameplayAbilitySpec* specC = FindAbilitySpecFromClass(m_AgentData->Ability_C);
	if (specC)
	{
		ClearAbility(specC->Handle);
	}
	FGameplayAbilitySpec* specE = FindAbilitySpecFromClass(m_AgentData->Ability_E);
	if (specE)
	{
		ClearAbility(specE->Handle);
	}
	FGameplayAbilitySpec* specQ = FindAbilitySpecFromClass(m_AgentData->Ability_Q);
	if (specQ)
	{
		ClearAbility(specQ->Handle);
	}
	FGameplayAbilitySpec* specX = FindAbilitySpecFromClass(m_AgentData->Ability_X);
	if (specX)
	{
		ClearAbility(specX->Handle);
	}
}
