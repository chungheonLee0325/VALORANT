#include "Jett_Q_Updraft.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UJett_Q_Updraft::UJett_Q_Updraft(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);
	m_AbilityID = 4002;
}
