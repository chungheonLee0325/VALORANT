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

	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));

	m_AbilityID = 1003;
}

void USage_E_HealingOrb::HandleLeftClick(FGameplayEventData EventData)
{
	Super::HandleLeftClick(EventData);

	// 라인트레이스로 아군 탐지
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner) return;
	FVector Start = Owner->GetActorLocation();
	FVector End = Start + Owner->GetActorForwardVector() * HealTraceDistance;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);
	FHitResult OutHit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		Start,
		End,
		UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel2), // TraceChannel: HitDetect
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		OutHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		2.5f
	);
	if (bHit)
	{
		ABaseAgent* Target = Cast<ABaseAgent>(OutHit.GetActor());
		ABaseAgent* OwnerAgent = Cast<ABaseAgent>(Owner);
		if (Target != nullptr && (Target->IsAttacker() == OwnerAgent->IsAttacker()))
		{
			// ToDo : 이펙트로 대체
			DrawDebugSphere(GetWorld(), OutHit.Location, 10.f, 32, FColor::Red, false, 2.f);

			// 힐링 이펙트/지속힐 적용 (Tick마다 HealAmount/HealDuration)
			Target->ServerApplyGE(GameplayEffect);
			TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
			CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
		}
	}
}

void USage_E_HealingOrb::HandleRightClick(FGameplayEventData EventData)
{
	Super::HandleRightClick(EventData);

	// 자신이 피해 입었을 때만 힐링
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent && !OwnerAgent->IsFullHealth())
	{
		// 힐링 이펙트/지속힐 적용 (Tick마다 HealAmount/HealDuration)
		OwnerAgent->ServerApplyGE(GameplayEffect);
		CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
		TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
	}
}
