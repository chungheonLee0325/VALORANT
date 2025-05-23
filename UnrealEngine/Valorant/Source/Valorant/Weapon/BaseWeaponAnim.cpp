// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapon/BaseWeaponAnim.h"

#include "ThirdPersonInteractor.h"
#include "Valorant.h"
#include "GameManager/SubsystemSteamManager.h"
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
