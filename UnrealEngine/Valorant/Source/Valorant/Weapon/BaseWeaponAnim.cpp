// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapon/BaseWeaponAnim.h"

#include "Weapon/BaseWeapon.h"

void UBaseWeaponAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	Owner = GetOwningActor();
	if (auto* Weapon = Cast<ABaseWeapon>(Owner))
	{
		Mesh = Weapon->GetMesh();
		Weapon->OnEquip.AddDynamic(this, &UBaseWeaponAnim::OnEquip);
		Weapon->OnFire.AddDynamic(this, &UBaseWeaponAnim::OnFire);
		Weapon->OnReload.AddDynamic(this, &UBaseWeaponAnim::OnReload);
	}
}