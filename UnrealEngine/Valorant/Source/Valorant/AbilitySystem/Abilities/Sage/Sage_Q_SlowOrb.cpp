#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"

USage_Q_SlowOrb::USage_Q_SlowOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 1002;
	InputType = EAbilityInputType::MultiPhase;

	// === 후속 입력 설정 (CDO에서 안전한 방식) ===
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
}

void USage_Q_SlowOrb::HandleLeftClick(FGameplayEventData data)
{
	SpawnProjectile(CachedActorInfo);
}