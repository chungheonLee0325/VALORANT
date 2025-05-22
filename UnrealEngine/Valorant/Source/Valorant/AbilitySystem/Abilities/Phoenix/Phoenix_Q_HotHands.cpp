#include "Phoenix_Q_HotHands.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"

UPhoenix_Q_HotHands::
UPhoenix_Q_HotHands(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 2002;

	InputType = EAbilityInputType::MultiPhase;

	SetFollowUpInputTag(FValorantGameplayTags::Get().InputTag_Default_LeftClick);
	
	// 타임아웃 설정
	PhaseTimeoutDuration = 5.0f; 
}

void UPhoenix_Q_HotHands::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPhoenix_Q_HotHands::Active_Left_Click(FGameplayEventData data)
{
	Super::Active_Left_Click(data);

	if (CurrentPhase == EAbilityPhase::Executing)
	{
		// 발사 애니메이션 재생
		if (ThrowMontage)
		{
			CurrentActorInfo->GetAnimInstance()->Montage_Play(ThrowMontage);
		}
            
		// 사운드 재생
		if (ThrowSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
		}
		
		// 발사 액션 (애니메이션 이벤트로 호출되거나 여기서 직접 호출)
		SpawnProjectile(m_ActorInfo);

		// 실행 단계로 전환
		SetupPhase(EAbilityPhase::Cooldown);
	}
}
