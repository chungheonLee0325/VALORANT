#include "Phoenix_Q_HotHands.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AgentAbility/BaseProjectile.h"

UPhoenix_Q_HotHands::UPhoenix_Q_HotHands() : UBaseGameplayAbility()
{
	// === 어빌리티 기본 설정 ===
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 2002;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
}

bool UPhoenix_Q_HotHands::OnLeftClickInput()
{
	bool bShouldExecute = true;
	// 직선 투사체 생성
	if (SpawnProjectileByType(EPhoenixQThrowType::Straight))
	{
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 투사체 생성 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 직선 투사체 생성 실패"));
		bShouldExecute = false;
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
	
	return bShouldExecute;
}

bool UPhoenix_Q_HotHands::OnRightClickInput()
{
	bool bShouldExecute = true;
	// 포물선 투사체 생성
	if (SpawnProjectileByType(EPhoenixQThrowType::Curved))
	{
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 투사체 생성 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 포물선 투사체 생성 실패"));
		bShouldExecute = false;
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
	return bShouldExecute;
}

bool UPhoenix_Q_HotHands::SpawnProjectileByType(EPhoenixQThrowType ThrowType)
{
	TSubclassOf<ABaseProjectile> ProjectileToSpawn = nullptr;

	switch (ThrowType)
	{
	case EPhoenixQThrowType::Straight:
		// ProjectileToSpawn = StraightProjectileClass ? StraightProjectileClass : ProjectileClass;
		break;
	case EPhoenixQThrowType::Curved:
		// ProjectileToSpawn = CurvedProjectileClass ? CurvedProjectileClass : ProjectileClass;
		break;
	default:
		ProjectileToSpawn = ProjectileClass;
		break;
	}
	ProjectileToSpawn = ProjectileClass;

	if (ProjectileToSpawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 투사체 클래스가 설정되지 않음"));
		return false;
	}

	// 기존 SpawnProjectile 로직을 사용하되, 투사체 클래스만 교체
	TSubclassOf<ABaseProjectile> OriginalProjectileClass = ProjectileClass;
	ProjectileClass = ProjectileToSpawn;

	bool bResult = SpawnProjectile();

	// 원래 클래스로 복원
	ProjectileClass = OriginalProjectileClass;

	return bResult;
}