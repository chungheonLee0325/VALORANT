#include "Brimstone_C_StimBeacon.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UBrimstone_C_StimBeacon::UBrimstone_C_StimBeacon(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	m_AbilityID = 5001;
}
