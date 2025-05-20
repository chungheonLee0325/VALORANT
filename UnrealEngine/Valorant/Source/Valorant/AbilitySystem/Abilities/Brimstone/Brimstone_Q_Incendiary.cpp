#include "Brimstone_Q_Incendiary.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UBrimstone_Q_Incendiary::UBrimstone_Q_Incendiary(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);
	m_AbilityID = 5002;
}
