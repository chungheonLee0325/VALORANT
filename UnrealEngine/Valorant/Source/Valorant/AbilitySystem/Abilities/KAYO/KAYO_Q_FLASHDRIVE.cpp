// Fill out your copyright notice in the Description page of Project Settings.


#include "KAYO_Q_FLASHDRIVE.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UKAYO_Q_FLASHDRIVE::UKAYO_Q_FLASHDRIVE(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);
	m_AbilityID = 3002;
}
