#include "Sage_E_HealingOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"

USage_E_HealingOrb::USage_E_HealingOrb(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 1003;
}
