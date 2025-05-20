// Fill out your copyright notice in the Description page of Project Settings.


#include "KAYO_E_ZEROPOINT.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UKAYO_E_ZEROPOINT::UKAYO_E_ZEROPOINT(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	m_AbilityID = 3003;
}
