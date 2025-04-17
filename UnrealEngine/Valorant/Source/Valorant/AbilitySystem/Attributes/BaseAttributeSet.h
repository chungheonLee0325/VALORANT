// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BaseAttributeSet.generated.h"

#define PLAY_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentHealthChanged, float, newHealth);
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentMaxHealthChanged, float, newMaxHealth);
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentArmorChanged, float, newArmor);
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentMoveSpeedChanged, float, newSpeed);

UCLASS()
class VALORANT_API UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UBaseAttributeSet();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	PLAY_ATTRIBUTE_ACCESSORS(UBaseAttributeSet,Health);

	UPROPERTY(BlueprintReadOnly, Category = "Agent", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	PLAY_ATTRIBUTE_ACCESSORS(UBaseAttributeSet,MaxHealth);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	PLAY_ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Armor);

	UPROPERTY(BlueprintReadOnly, Category = "Agent")
	FGameplayAttributeData MaxArmor;
	PLAY_ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxArmor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	PLAY_ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MoveSpeed);
	
	// UPROPERTY(BlueprintAssignable)
	// FOnAgentHealthChanged OnAgentHealthChanged;
	// UPROPERTY(BlueprintAssignable)
	// FOnAgentMaxHealthChanged OnAgentMaxHealthChanged;
	// UPROPERTY(BlueprintAssignable)
	// FOnAgentArmorChanged OnAgentArmorChanged;
	// UPROPERTY(BlueprintAssignable)
	// FOnAgentMoveSpeedChanged OnAgentMoveSpeedChanged;
	
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
};
