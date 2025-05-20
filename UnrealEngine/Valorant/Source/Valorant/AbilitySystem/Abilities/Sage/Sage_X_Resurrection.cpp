#include "Sage_X_Resurrection.h"
#include "AbilitySystem/ValorantGameplayTags.h"

USage_X_Resurrection::USage_X_Resurrection(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")));
	SetAssetTags(Tags);

	m_AbilityID = 1004;
}
