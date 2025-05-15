// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeKnife.h"

#include "Valorant.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/Agent/BaseAgent.h"


// Sets default values
AMeleeKnife::AMeleeKnife()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMeleeKnife::BeginPlay()
{
	Super::BeginPlay();
	InteractorType = EInteractorType::Melee;
}

void AMeleeKnife::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AMeleeKnife::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	return false;
}

bool AMeleeKnife::ServerOnly_CanDrop() const
{
	return false;
}

void AMeleeKnife::StartFire()
{
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetController())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OwnerAgent or Controller is nullptr"), __FUNCTION__);
		return;
	}
	
	// 플레이 중인 애니메이션이 있고, 아직 콤보 상태에 진입 전이면, 입력 무시
	if (bIsAttacking)
	{
		if (bIsCombo == false)
		{
			// NET_LOG(LogTemp, Warning, TEXT("입력 무시"));
			return;
		}
	}
	
	Fire();
}

void AMeleeKnife::Fire()
{
	// NET_LOG(LogTemp, Warning, TEXT("장탄 상태 = %d."), MagazineAmmo);

	bIsComboTransition = true;
	
	switch (MagazineAmmo)
	{
	case 3:
		Server_PlayAttackAnim(AM_Fire);
		break;
	case 2:
		Server_PlayAttackAnim(AM_Fire2);
		break;
	case 1:
		Server_PlayAttackAnim(AM_Fire3);
		break;
	default:
		break;
	}
}

void AMeleeKnife::ResetCombo()
{
	bIsAttacking = false;
	bIsCombo = false;
	MagazineAmmo = MagazineSize;
}

void AMeleeKnife::OnMontageEnded(UAnimMontage* AnimMontage, bool bInterrupted)
{
	//콤보에 의해 애니메이션이 끊기면
	if (bInterrupted && bIsComboTransition)
	{
		// NET_LOG(LogTemp,Warning,TEXT("콤보에 의해 기존 몽타주 종료"));
		bIsComboTransition = false;
		return;
	}
	
	// NET_LOG(LogTemp,Warning,TEXT("몽타주 정상 종료"));
	ResetCombo();
}

void AMeleeKnife::Server_PlayAttackAnim_Implementation(UAnimMontage* anim)
{
	if (!anim)
	{
		return;
	}
	
	Multicast_PlayAttackAnim(anim);
}

void AMeleeKnife::Multicast_PlayAttackAnim_Implementation(UAnimMontage* anim)
{
	MagazineAmmo--;
	bIsAttacking = true;

	if (OwnerAgent == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = nullptr;
	
	if (OwnerAgent->IsLocallyControlled())
	{
		AnimInstance = OwnerAgent->GetABP_1P();
	}
	else
	{
		AnimInstance = OwnerAgent->GetABP_3P();
	}
	
	if (!AnimInstance)
	{
		return;
	}

	float Duration = AnimInstance->Montage_Play(anim, 1.0f);

	if (Duration > 0.f && OwnerAgent->IsLocallyControlled())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AMeleeKnife::OnMontageEnded);
	
		AnimInstance->Montage_SetEndDelegate(EndDelegate, anim);
	}
}
