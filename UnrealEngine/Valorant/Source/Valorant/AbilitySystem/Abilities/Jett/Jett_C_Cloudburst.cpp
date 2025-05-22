#include "Jett_C_Cloudburst.h"
#include "AbilitySystem/ValorantGameplayTags.h"

UJett_C_Cloudburst::UJett_C_Cloudburst(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	m_AbilityID = 4001;
}

void UJett_C_Cloudburst::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ActorInfo->GetAnimInstance()->Montage_Play(AbilityMontage);

	// 타이머 할지 시간 보기
	SpawnProjectile(CachedActorInfo);
}
