// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeKnife.h"

#include "Valorant.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/Agent/BaseAgent.h"


// Sets default values
AMeleeKnife::AMeleeKnife()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractorType = EInteractorType::Melee;
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
		Server_PlayAttackAnim(AM_Fire1_1P, AM_Fire1_3P);
		break;
	case 2:
		Server_PlayAttackAnim(AM_Fire2_1P, AM_Fire2_3P);
		break;
	case 1:
		Server_PlayAttackAnim(AM_Fire3_1P, AM_Fire3_3P);
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

void AMeleeKnife::Server_PlayAttackAnim_Implementation(UAnimMontage* anim1P, UAnimMontage* anim3P)
{
	if (!anim1P || !anim3P)
	{
		return;
	}
	
	Multicast_PlayAttackAnim(anim1P, anim3P);
}

void AMeleeKnife::Multicast_PlayAttackAnim_Implementation(UAnimMontage* anim1P, UAnimMontage* anim3P)
{
	MagazineAmmo--;
	bIsAttacking = true;

	if (OwnerAgent == nullptr)
	{
		return;
	}
	
	UAnimInstance* abp3P = OwnerAgent->GetABP_3P();
	if (OwnerAgent->GetABP_1P() == nullptr)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, ABP 3p is nullptr"), __FUNCTION__);
	}
	
	if (!abp3P)
	{
		return;
	}
	
	float Duration = abp3P->Montage_Play(anim3P, 1.0f);
	
	if (Duration > 0.f && OwnerAgent->IsLocallyControlled())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AMeleeKnife::OnMontageEnded);
	
		abp3P->Montage_SetEndDelegate(EndDelegate, anim3P);
	}

	
		UAnimInstance* abp = OwnerAgent->GetABP_1P();
		if (abp == nullptr)
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, ABP 1p is nullptr"), __FUNCTION__);
		}

		abp->Montage_Play(anim1P, 1.0f);
	
}
