#include "Brimstone_E_SkySmoke.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UBrimstone_E_SkySmoke::UBrimstone_E_SkySmoke(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);
	m_AbilityID = 5003;
}
