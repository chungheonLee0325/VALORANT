#include "Jett_C_Cloudburst.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UJett_C_Cloudburst::UJett_C_Cloudburst(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	
	m_AbilityID = 4001;
	ActivationType = EAbilityActivationType::Instant;
}

void UJett_C_Cloudburst::ExecuteAbility()
{
	SpawnProjectile();
}
