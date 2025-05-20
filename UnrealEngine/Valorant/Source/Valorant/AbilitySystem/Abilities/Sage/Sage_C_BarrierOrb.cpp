#include "Sage_C_BarrierOrb.h" 

USage_C_BarrierOrb::USage_C_BarrierOrb(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	
	m_AbilityID = 1001;
}
