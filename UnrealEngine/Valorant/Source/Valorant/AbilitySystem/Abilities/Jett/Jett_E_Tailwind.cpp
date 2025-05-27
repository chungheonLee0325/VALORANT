#include "Jett_E_Tailwind.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ResourceManager/ValorantGameType.h"

UJett_E_Tailwind::UJett_E_Tailwind(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 4003;
	ActivationType = EAbilityActivationType::Instant;

	// ToDo : 추후 구현
	// ActivationType = EAbilityActivationType::WithPrepare;
	// FollowUpInputType = EFollowUpInputType::RepeatKey;
}

void UJett_E_Tailwind::ExecuteAbility()
{
	ACharacter* Character = Cast<ACharacter>(CachedActorInfo.AvatarActor.Get());
	if (Character)
	{
		UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
		if (MoveComp)
		{
			// 돌진 방향: 캐릭터가 바라보는 방향 (XY 평면)
			FVector Forward = Character->GetActorForwardVector();
			Forward.Z = 0.f;
			Forward.Normalize();

			const float DashStrength = 4000.0f; // 돌진 세기
			FVector DashVelocity = Forward * DashStrength;

			// 공중/지상 모두에서 동일하게 돌진: Z속도는 기존값 유지(또는 0으로 보정)
			DashVelocity.Z = 0.f; // 돌진시 위/아래로 튀지 않게 보정
			MoveComp->Velocity = FVector::ZeroVector; // 기존 속도 제거(중첩 방지)

			// 마찰력/브레이킹 저장 및 0으로 설정
			float OriginalFriction = MoveComp->GroundFriction;
			float OriginalBraking = MoveComp->BrakingFrictionFactor;
			MoveComp->GroundFriction = 0.f;
			MoveComp->BrakingFrictionFactor = 0.f;

			Character->LaunchCharacter(DashVelocity, true, true);

			// 0.2초 후 원래 값 복구
			FTimerHandle TimerHandle;
			Character->GetWorldTimerManager().SetTimer(TimerHandle, [MoveComp, OriginalFriction, OriginalBraking]()
			{
				MoveComp->GroundFriction = OriginalFriction;
				MoveComp->BrakingFrictionFactor = OriginalBraking;
			}, 0.3f, false);
		}
	}
}
