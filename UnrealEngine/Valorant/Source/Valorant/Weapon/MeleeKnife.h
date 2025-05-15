// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseWeapon.h"
#include "MeleeKnife.generated.h"

UCLASS()
class VALORANT_API AMeleeKnife : public ABaseWeapon
{
	GENERATED_BODY()

public:
	AMeleeKnife();
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UAnimMontage* AM_Fire2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UAnimMontage* AM_Fire3;

	bool bIsAttacking = false;
	bool bIsCombo = false;
	bool bIsComboTransition = false;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const override;
	virtual bool ServerOnly_CanDrop() const override;

	virtual void StartFire() override;
	virtual void Fire() override;

	void ResetCombo();

	UFUNCTION(Server, Reliable)
	void Server_PlayAttackAnim(UAnimMontage* anim);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackAnim(UAnimMontage* anim);

	// void MeleeAnimPlay(UAnimInstance* animInstance, UAnimMontage* anim);
	
	void OnMontageEnded(UAnimMontage* AnimMontage, bool bInterrupted);
};
