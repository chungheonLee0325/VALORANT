#include "Sage_E_HealingOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Player/Agent/BaseAgent.h"

USage_E_HealingOrb::USage_E_HealingOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	//FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
	//FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));

	m_AbilityID = 1003;
}

void USage_E_HealingOrb::Active_Left_Click(FGameplayEventData data)
{
	// 라인트레이스로 아군 탐지
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner) return;
	FVector Start = Owner->GetActorLocation();
	FVector End = Start + Owner->GetActorForwardVector() * HealTraceDistance;
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	if (Owner->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		ABaseAgent* Target = Cast<ABaseAgent>(Hit.GetActor());
		ABaseAgent* OwnerAgent = Cast<ABaseAgent>(Owner);
		if (Target != nullptr && (Target->IsAttacker() == OwnerAgent->IsAttacker()))
		{
			// 힐링 이펙트/지속힐 적용 (Tick마다 HealAmount/HealDuration)
			Target->ServerApplyGE(GameplayEffect);
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USage_E_HealingOrb::Active_Right_Click(FGameplayEventData data)
{
	// 자신이 피해 입었을 때만 힐링
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent /* && 체력 체크 */)
	{
		// 힐링 이펙트/지속힐 적용 (Tick마다 HealAmount/HealDuration)
		OwnerAgent->ServerApplyGE(GameplayEffect);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
