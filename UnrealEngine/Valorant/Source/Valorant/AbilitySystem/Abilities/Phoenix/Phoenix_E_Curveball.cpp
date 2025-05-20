#include "Phoenix_E_Curveball.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UPhoenix_E_Curveball::UPhoenix_E_Curveball(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 2003;
}
