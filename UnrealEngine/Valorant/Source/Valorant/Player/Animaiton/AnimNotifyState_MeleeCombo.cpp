// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_MeleeCombo.h"

#include "Player/Agent/BaseAgent.h"
#include "Weapon/Melee/MeleeKnife.h"

void UAnimNotifyState_MeleeCombo::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                              float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (AActor* owner = MeshComp->GetOwner())
	{
		auto* agent = Cast<ABaseAgent>(owner);
		if (agent == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("애님노티파이 소유자가 BaseAgent를 상속받아야 합니다."));
			return;
		}
		if (AMeleeKnife* knife = Cast<AMeleeKnife>(agent->GetMeleeWeapon()))
		{
			knife->bIsCombo = true;
			// UE_LOG(LogTemp, Error, TEXT("애님노티파이, 콤보 true"));
		}
	}
}

void UAnimNotifyState_MeleeCombo::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);
	if (AActor* owner = MeshComp->GetOwner())
	{
		auto* agent = Cast<ABaseAgent>(owner);
		if (agent == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("애님노티파이 소유자가 BaseAgent를 상속받아야 합니다."));
			return;
		}
		if (AMeleeKnife* knife = Cast<AMeleeKnife>(agent->GetMeleeWeapon()))
		{
			knife->bIsCombo = false;
			// UE_LOG(LogTemp, Error, TEXT("애님노티파이, 콤보 false"));
		}
	}
}
