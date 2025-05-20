#include "Phoenix_X_RunItBack.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UPhoenix_X_RunItBack::UPhoenix_X_RunItBack(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")));
	SetAssetTags(Tags);
	m_AbilityID = 2004;
}
