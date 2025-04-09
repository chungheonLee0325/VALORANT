// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AgentAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class VALORANT_API UAgentAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UAgentAttributeSet();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet,Health);

	UPROPERTY(BlueprintReadOnly, Category = "Agent")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet,MaxHealth);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, Armor);

	UPROPERTY(BlueprintReadOnly, Category = "Agent")
	FGameplayAttributeData MaxArmor;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, MaxArmor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, MoveSpeed);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
	FGameplayAttributeData Creds;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, Creds);

private:
	//Tag는 private
	
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);


};
