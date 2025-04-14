// Fill out your copyright notice in the Description page of Project Settings.


#include "JettAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UJettAttributeSet::UJettAttributeSet()
	: TailwindCharges(1.0f) // Default value
	, TailwindMaxCharges(1.0f) // Default value
{
}
void UJettAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Enable replication for attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UJettAttributeSet, TailwindCharges, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJettAttributeSet, TailwindMaxCharges, COND_None, REPNOTIFY_Always);
}

void UJettAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamp TailwindCharges between 0 and TailwindMaxCharges
	if (Attribute == GetTailwindChargesAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetTailwindMaxCharges());
	}
}

void UJettAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Example: If Health is changed, you might check for death here
	// For TailwindCharges, clamping is handled in PreAttributeChange, but you could add checks here too.
	if (Data.EvaluatedData.Attribute == GetTailwindChargesAttribute())
	{
		// Ensure charges don't go below 0 or above max after effect application
		SetTailwindCharges(FMath::Clamp(GetTailwindCharges(), 0.0f, GetTailwindMaxCharges()));
	}
}

void UJettAttributeSet::OnRep_TailwindCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJettAttributeSet, TailwindCharges, OldValue);
}

void UJettAttributeSet::OnRep_TailwindMaxCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJettAttributeSet, TailwindMaxCharges, OldValue);
}