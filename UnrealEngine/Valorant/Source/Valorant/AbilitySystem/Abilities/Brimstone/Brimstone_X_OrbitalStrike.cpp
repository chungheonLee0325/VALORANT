#include "Brimstone_X_OrbitalStrike.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UBrimstone_X_OrbitalStrike::UBrimstone_X_OrbitalStrike(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")));
	SetAssetTags(Tags);
	m_AbilityID = 5004;
}
