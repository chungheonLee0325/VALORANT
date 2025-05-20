#include "Jett_E_Tailwind.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UJett_E_Tailwind::UJett_E_Tailwind(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);
	m_AbilityID = 4003;
}
