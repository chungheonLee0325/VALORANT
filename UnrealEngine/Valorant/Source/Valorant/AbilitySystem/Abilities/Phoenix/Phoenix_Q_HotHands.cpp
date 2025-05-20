#include "Phoenix_Q_HotHands.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UPhoenix_Q_HotHands::UPhoenix_Q_HotHands(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 2002;
}
