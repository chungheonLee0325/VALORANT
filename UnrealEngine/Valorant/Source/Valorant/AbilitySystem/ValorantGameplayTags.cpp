// Fill out your copyright notice in the Description page of Project Settings.


#include "ValorantGameplayTags.h"

#include "GameplayTagsManager.h"

#define REGISTER_TAG(TagVar, TagNameStr) TagVar = TagManager.RequestGameplayTag(FName(TagNameStr));

FValorantGameplayTags& FValorantGameplayTags::Get()
{
	static FValorantGameplayTags Instance;
	return Instance;
}

void FValorantGameplayTags::InitializeNativeTags()
{
	auto& TagManager = UGameplayTagsManager::Get();

	REGISTER_TAG(InputTag_Ability_Q, "Input.Skill.Q");
	REGISTER_TAG(InputTag_Ability_E, "Input.Skill.E");
	REGISTER_TAG(InputTag_Ability_C, "Input.Skill.C");
	REGISTER_TAG(InputTag_Ability_X, "Input.Skill.X");
	REGISTER_TAG(InputTag_Default_LeftClick, "Input.Default.LeftClick");
	REGISTER_TAG(InputTag_Default_RightClick, "Input.Default.RightClick");
}
