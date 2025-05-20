#include "Jett_X_BladeStorm.h" 
#include "AbilitySystem/ValorantGameplayTags.h"

UJett_X_BladeStorm::UJett_X_BladeStorm(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")));
	SetAssetTags(Tags);
	m_AbilityID = 4004;
}
