// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * 
 */

struct FValorantGameplayTags
{
	FGameplayTag InputTag_Ability_Q;
	FGameplayTag InputTag_Ability_E;
	FGameplayTag InputTag_Ability_C;
	FGameplayTag InputTag_Ability_X;

	FGameplayTag InputTag_Default_LeftClick;
	FGameplayTag InputTag_Default_RightClick;
	
	static FValorantGameplayTags& Get();
	void InitializeNativeTags();
};
