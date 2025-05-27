#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"

USage_Q_SlowOrb::USage_Q_SlowOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 1002;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
}

bool USage_Q_SlowOrb::OnLeftClickInput()
{
	SpawnProjectile();
	return true;
}
