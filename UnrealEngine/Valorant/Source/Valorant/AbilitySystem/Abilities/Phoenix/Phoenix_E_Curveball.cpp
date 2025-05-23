#include "Phoenix_E_Curveball.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/FlashProjectile.h"

UPhoenix_E_Curveball::UPhoenix_E_Curveball(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 2003;
	InputType = EAbilityInputType::MultiPhase;

	// === 후속 입력 설정 (CDO에서 안전한 방식) ===
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));
}

void UPhoenix_E_Curveball::HandleLeftClick(FGameplayEventData EventData)
{
	Super::HandleLeftClick(EventData);

	// 섬광탄 발사
	SpawnFlashProjectile();

    
	// 어빌리티 완료
	TransitionToState(FValorantGameplayTags::Get().State_Ability_Ended);
	CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}

bool UPhoenix_E_Curveball::SpawnFlashProjectile()
{
	if (!FlashProjectileClass)
		return false;
	
	// BaseGameplayAbility의 ProjectileClass 설정
	ProjectileClass = FlashProjectileClass;
    
	// 기본 SpawnProjectile 사용
	return SpawnProjectile(CachedActorInfo);
}
