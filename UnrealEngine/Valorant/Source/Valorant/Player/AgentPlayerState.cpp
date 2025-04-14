// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerState.h"

#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Attribute/BaseAttributeSet.h"

AAgentPlayerState::AAgentPlayerState()
{
	ASC = CreateDefaultSubobject<UAgentAbilitySystemComponent>(TEXT("ASC"));
	ASC-> SetIsReplicated(true);

	// GE에 의해 값이 변경될 때, GE 인스턴스를 복제할 범위 설정
	// Full - 항상 복제 / Minimal - GE에 의해 변경되는 AttributeData 값만 복제 / Mixed - 혼합
	// AttributeData에 ReplicatedUsing 설정을 하는 것과는 별개의 개념.
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
	
	NetUpdateFrequency = 100.f;
	MinNetUpdateFrequency = 33.f;
}

UAbilitySystemComponent* AAgentPlayerState::GetAbilitySystemComponent() const
{
	return ASC;
}

UBaseAttributeSet* AAgentPlayerState::GetBaseAttributeSetBase() const
{
	return BaseAttributeSet;
}

float AAgentPlayerState::GetHealth() const
{
	return BaseAttributeSet->GetHealth();
}

float AAgentPlayerState::GetMaxHealth() const
{
	return BaseAttributeSet->GetMaxHealth();
}

float AAgentPlayerState::GetArmor() const
{
	return BaseAttributeSet->GetArmor();
}

float AAgentPlayerState::GetMoveSpeed() const
{
	return BaseAttributeSet->GetMoveSpeed();
}
