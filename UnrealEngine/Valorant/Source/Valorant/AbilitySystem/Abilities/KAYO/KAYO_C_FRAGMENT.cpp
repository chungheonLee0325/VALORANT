#include "KAYO_C_FRAGMENT.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UKAYO_C_FRAGMENT::UKAYO_C_FRAGMENT(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	m_AbilityID = 3001;
}
