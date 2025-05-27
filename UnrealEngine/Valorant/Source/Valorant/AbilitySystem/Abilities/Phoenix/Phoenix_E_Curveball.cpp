#include "Phoenix_E_Curveball.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/FlashProjectile.h"
#include "AgentAbility/KayO/Flashbang.h"

UPhoenix_E_Curveball::UPhoenix_E_Curveball(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 2003;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;

	// 태그 설정
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Phoenix.Curveball")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
}

bool UPhoenix_E_Curveball::OnLeftClickInput()
{
	bool bShouldExecute = true;
	
	// 섬광탄 발사
	SpawnFlashProjectile(false);

	return bShouldExecute;
}

bool UPhoenix_E_Curveball::OnRightClickInput()
{
	bool bShouldExecute = true;

	// 섬광탄 발사
	SpawnFlashProjectile(true);


	return bShouldExecute;
}

bool UPhoenix_E_Curveball::SpawnFlashProjectile(bool IsUnder)
{
	if (!FlashProjectileClass)
		return false;
	
	// BaseGameplayAbility의 ProjectileClass 설정
	ProjectileClass = FlashProjectileClass;
    
	// 기본 SpawnProjectile 사용
	bool result = SpawnProjectile();
	if (auto flashBang = Cast<AFlashbang>(SpawnedProjectile))
	{
		flashBang->ActiveProjectileMovement(IsUnder);
	}
	return result;
}
