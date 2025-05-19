// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ResourceManager/ValorantGameType.h"
#include "AgentAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UAgentAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Move)
	float Speed = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Move)
	float Direction = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Look)
	float Pitch = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Look)
	float Yaw = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State)
	bool bIsInAir = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State)
	bool bIsCrouch = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State)
	bool bIsDead = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Interactor)
	EInteractorType InteractorState = EInteractorType::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Interactor)
	int InteractorPoseIdx = 0;
	
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	void SetWeaponState(const EInteractorType newState) { InteractorState = newState; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquip();
	UFUNCTION(BlueprintImplementableEvent)
	void OnFire();
	UFUNCTION(BlueprintImplementableEvent)
	void OnReload();
};
