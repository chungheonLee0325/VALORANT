// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Valorant/Attribute/BaseAttributeSet.h"
#include "AbilitySystemComponent.h" 
#include "JettAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UJettAttributeSet : public UBaseAttributeSet
{
	GENERATED_BODY()

public:
	UJettAttributeSet();

	// AttributeSet Overrides
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// --- Attributes ---

	// Current Tailwind charges
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Jett", ReplicatedUsing = OnRep_TailwindCharges)
	FGameplayAttributeData TailwindCharges;
	PLAY_ATTRIBUTE_ACCESSORS(UJettAttributeSet, TailwindCharges);

	// Max Tailwind charges
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Jett", ReplicatedUsing = OnRep_TailwindMaxCharges)
	FGameplayAttributeData TailwindMaxCharges;
	PLAY_ATTRIBUTE_ACCESSORS(UJettAttributeSet, TailwindMaxCharges);

	// --- RepNotifies ---
protected:
	UFUNCTION()
	virtual void OnRep_TailwindCharges(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_TailwindMaxCharges(const FGameplayAttributeData& OldValue);
};

