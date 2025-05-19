// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BaseWeaponAnim.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UBaseWeaponAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<AActor> Owner = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bThirdPerson = false;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnEquip();
	UFUNCTION(BlueprintImplementableEvent)
	void OnFire();
	UFUNCTION(BlueprintImplementableEvent)
	void OnReload();

protected:
	virtual void NativeInitializeAnimation() override;
};
