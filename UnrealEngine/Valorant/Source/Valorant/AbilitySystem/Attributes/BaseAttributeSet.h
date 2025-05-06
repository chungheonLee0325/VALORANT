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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, newHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChanged, float, newMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmorChanged, float, newArmor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectSpeedChanged, float, newEffectSpeed);

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", ReplicatedUsing = OnRep_EffectSpeed)
	FGameplayAttributeData EffectSpeedMultiplier = 1.0f;
	PLAY_ATTRIBUTE_ACCESSORS(UBaseAttributeSet, EffectSpeedMultiplier);

	//통합 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnMaxHealthChanged OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnArmorChanged OnArmorChanged;
	UPROPERTY(BlueprintAssignable)
	FOnEffectSpeedChanged OnEffectSpeedChanged;

	//수동 변경 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged_Manual;
	UPROPERTY(BlueprintAssignable)
	FOnMaxHealthChanged OnMaxHealthChanged_Manual;
	UPROPERTY(BlueprintAssignable)
	FOnArmorChanged OnArmorChanged_Manual;
	UPROPERTY(BlueprintAssignable)
	FOnEffectSpeedChanged OnEffectSpeedChanged_Manual;

	//이펙트 변경 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged_FromGE;
	UPROPERTY(BlueprintAssignable)
	FOnMaxHealthChanged OnMaxHealthChanged_FromGE;
	UPROPERTY(BlueprintAssignable)
	FOnArmorChanged OnArmorChanged_FromGE;
	UPROPERTY(BlueprintAssignable)
	FOnEffectSpeedChanged OnEffectSpeedChanged_FromGE;

	
public:
	UFUNCTION(BlueprintCallable)
	virtual void ResetAttributeData();
	
protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);
	
	UFUNCTION()
	virtual void OnRep_EffectSpeed(const FGameplayAttributeData& OldEffectSpeed);

	virtual void ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const;
};
