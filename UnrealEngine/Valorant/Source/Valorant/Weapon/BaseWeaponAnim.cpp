// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapon/BaseWeaponAnim.h"

#include "ThirdPersonInteractor.h"
#include "Valorant.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Weapon/BaseWeapon.h"

void UBaseWeaponAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Owner = GetOwningActor();
	auto* Weapon = Cast<ABaseWeapon>(Owner);
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
			return;
		}
	}
	Weapon->OnEquip.AddDynamic(this, &UBaseWeaponAnim::OnEquip);
	Weapon->OnFire.AddDynamic(this, &UBaseWeaponAnim::OnFire);
	Weapon->OnReload.AddDynamic(this, &UBaseWeaponAnim::OnReload);
	Weapon->OnPickUp.AddDynamic(this, &UBaseWeaponAnim::OnPickUp);
	Weapon->OnInteractorDrop.AddDynamic(this, &UBaseWeaponAnim::OnDrop);
}