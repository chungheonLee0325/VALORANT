// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_MeleeReset.h"

#include "Player/Agent/BaseAgent.h"
#include "Weapon/Melee/MeleeKnife.h"

class AMeleeKnife;

void UAnimNotify_MeleeReset::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                    const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
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
			knife->ResetCombo();
		}
	}
}
