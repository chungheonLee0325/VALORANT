// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAttributeSet.h"
#include "Valorant/Agent/BaseAgent.h"
#include "Net/UnrealNetwork.h"

UAgentAttributeSet::UAgentAttributeSet()
{
	//HitDirectionFrontTag = FGameplayTag::RequestGameplayTag(FName("Effect.HitReact.Front"));
}

void UAgentAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		// Cannot slow less than 150 units/s and cannot boost more than 1000 units/s
		NewValue = FMath::Clamp<float>(NewValue, 150, 1000);
	}
}

void UAgentAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void UAgentAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UAgentAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute,
	const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	
}

void UAgentAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
}

void UAgentAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldHealth)
{
}

void UAgentAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldHealth)
{
}
