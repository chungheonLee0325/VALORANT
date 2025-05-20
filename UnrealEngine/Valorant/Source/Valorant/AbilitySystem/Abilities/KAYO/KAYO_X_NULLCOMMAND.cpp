// Fill out your copyright notice in the Description page of Project Settings.


#include "KAYO_X_NULLCOMMAND.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UKAYO_X_NULLCOMMAND::UKAYO_X_NULLCOMMAND(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")));
	SetAssetTags(Tags);
	m_AbilityID = 3004;
}
