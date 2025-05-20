#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"

USage_Q_SlowOrb::USage_Q_SlowOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));

	m_AbilityID = 1002;
}

void USage_Q_SlowOrb::Active_Left_Click(FGameplayEventData data)
{
	SpawnProjectile(m_ActorInfo);
}