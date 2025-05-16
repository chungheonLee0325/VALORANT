// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SpeedStimAnim.generated.h"

UENUM(BlueprintType)
enum class ESpeedStimState : uint8
{
	ESSS_Idle,
	ESSS_Active,
	ESSS_Outtro
};

/**
 * 
 */
UCLASS()
class VALORANT_API USpeedStimAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESpeedStimState State = ESpeedStimState::ESSS_Idle;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeploy();
};
