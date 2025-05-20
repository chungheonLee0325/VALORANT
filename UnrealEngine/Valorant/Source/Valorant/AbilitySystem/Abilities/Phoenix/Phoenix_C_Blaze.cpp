#include "Phoenix_C_Blaze.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UPhoenix_C_Blaze::UPhoenix_C_Blaze(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);

	m_AbilityID = 2001;
}
