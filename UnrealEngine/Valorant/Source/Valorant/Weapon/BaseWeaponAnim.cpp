// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapon/BaseWeaponAnim.h"

#include "ThirdPersonInteractor.h"
#include "Valorant.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/Agent/BaseAgent.h"
#include "Weapon/BaseWeapon.h"

void UBaseWeaponAnim::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	Owner = GetOwningActor();
	Weapon = Cast<ABaseWeapon>(Owner);
	auto* ThirdPersonWeapon = Cast<AThirdPersonInteractor>(Owner);
	if (Weapon)
	{
		Mesh = Weapon->GetMesh();
	}
	else if (ThirdPersonWeapon)
	{
		Mesh = ThirdPersonWeapon->Mesh;
		bThirdPerson = true;
		Weapon = Cast<ABaseWeapon>(ThirdPersonWeapon->OwnerInteractor);
		if (nullptr == Weapon)
		{
			NET_LOG(LogTemp, Error, TEXT("%hs Called, Owner is not weapon1"), __FUNCTION__);
			return;
		}
	}
	else
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, Owner is not weapon2"), __FUNCTION__);
		return;
	}
	
	Weapon->OnEquip.AddDynamic(this, &UBaseWeaponAnim::OnEquip);
	Weapon->OnFire.AddDynamic(this, &UBaseWeaponAnim::OnFire);
	Weapon->OnReload.AddDynamic(this, &UBaseWeaponAnim::OnReload);
	Weapon->OnPickUp.AddDynamic(this, &UBaseWeaponAnim::OnPickUp);
	Weapon->OnInteractorDrop.AddDynamic(this, &UBaseWeaponAnim::OnDrop);
}

bool UBaseWeaponAnim::ShouldPlayAnim() const
{
	if (nullptr == Weapon->GetOwnerAgent())
	{
		return false;
	}
	
	if (bThirdPerson)
	{
		if (true == Weapon->GetOwnerAgent()->IsLocallyControlled())
		{
			// 3인칭 무기인데 내가 조작하고 있는 에이전트의 무기라면 애니메이션 실행하지 않는다
			return false;
		}
		
	}
	else
	{
		if (false == Weapon->GetOwnerAgent()->IsLocallyControlled())
		{
			// 1인칭 무기인데 내 무기가 아니라면 애니메이션을 실행하지 않는다
			return false;
		}
	}
	
	return true;
}

void UBaseWeaponAnim::OnHandleEquip()
{
	if (ShouldPlayAnim()) OnEquip();
}

void UBaseWeaponAnim::OnHandleFire()
{
	if (ShouldPlayAnim()) OnFire();
}

void UBaseWeaponAnim::OnHandleReload()
{
	if (ShouldPlayAnim()) OnReload();
}

void UBaseWeaponAnim::OnHandlePickUp()
{
	if (ShouldPlayAnim()) OnPickUp();
}

void UBaseWeaponAnim::OnHandleDrop()
{
	if (ShouldPlayAnim()) OnDrop();
}

void UBaseWeaponAnim::SomethingWrong()
{
	if (Weapon)
	{
		Weapon->OnEquip.RemoveAll(this);
		Weapon->OnFire.RemoveAll(this);
		Weapon->OnReload.RemoveAll(this);
		Weapon->OnPickUp.RemoveAll(this);
		Weapon->OnInteractorDrop.RemoveAll(this);
		Owner = nullptr;
		Weapon = nullptr;
		Mesh = nullptr;
	}
}
