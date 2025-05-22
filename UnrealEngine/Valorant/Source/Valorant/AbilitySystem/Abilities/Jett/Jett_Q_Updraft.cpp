#include "Jett_Q_Updraft.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UJett_Q_Updraft::UJett_Q_Updraft(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 4002;
	InputType = EAbilityInputType::Instant;
}

void UJett_Q_Updraft::HandleExecutingState()
{
	Super::HandleExecutingState();

	ACharacter* Character = Cast<ACharacter>(CachedActorInfo.AvatarActor.Get());
	if (Character)
	{
		CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);

		UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
		if (MoveComp)
		{
			const float UpdraftStrength = 4000.0f;
			FVector LaunchVelocity = FVector(0, 0, UpdraftStrength);
			MoveComp->Velocity.Z = 0;
			Character->LaunchCharacter(LaunchVelocity, true, true);

			// 중력/브레이킹 일시적으로 증가
			float OriginalGravity = MoveComp->GravityScale;
			float OriginalBraking = MoveComp->BrakingDecelerationFalling;
			MoveComp->GravityScale = 12.f;
			MoveComp->BrakingDecelerationFalling = 8000.0f;

			// 0.3초 후 원래 값 복구
			FTimerHandle TimerHandle;
			Character->GetWorldTimerManager().SetTimer(TimerHandle, [MoveComp, OriginalGravity, OriginalBraking]()
			{
				MoveComp->GravityScale = OriginalGravity;
				MoveComp->BrakingDecelerationFalling = OriginalBraking;
			}, 0.3f, false);
		}
	}
}
