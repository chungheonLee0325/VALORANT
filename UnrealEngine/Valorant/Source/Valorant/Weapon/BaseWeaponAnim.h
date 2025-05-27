// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BaseWeaponAnim.generated.h"

class ABaseWeapon;
class USkeletalMeshComponent;

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
	TObjectPtr<ABaseWeapon> Weapon = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bThirdPerson = false;
	
	UFUNCTION()
	void OnHandleEquip();
	UFUNCTION()
	void OnHandleFire();
	UFUNCTION()
	void OnHandleReload();
	UFUNCTION()
	void OnHandlePickUp();
	UFUNCTION()
	void OnHandleDrop();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquip();
	UFUNCTION(BlueprintImplementableEvent)
	void OnFire();
	UFUNCTION(BlueprintImplementableEvent)
	void OnReload();
	UFUNCTION(BlueprintImplementableEvent)
	void OnPickUp();
	UFUNCTION(BlueprintImplementableEvent)
	void OnDrop();

protected:
	virtual void NativeBeginPlay() override;
	bool ShouldPlayAnim() const;
	UFUNCTION(BlueprintCallable)
	void SomethingWrong();
};
