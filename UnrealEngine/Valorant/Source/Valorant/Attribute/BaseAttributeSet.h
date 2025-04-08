// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BaseAttributeSet.generated.h"

#define PLAY_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayAttributeEvent, float, EffectMagnitude, float, NewValue);

UCLASS()
class VALORANT_API UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
};
