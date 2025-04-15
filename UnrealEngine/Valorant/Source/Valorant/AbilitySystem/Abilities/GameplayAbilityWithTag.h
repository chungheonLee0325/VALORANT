// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseGameplayAbility.h"
#include "GameplayAbilityWithTag.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UGameplayAbilityWithTag : public UBaseGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category="Tags")
	FGameplayTag AbilityTypeTag; 
};
