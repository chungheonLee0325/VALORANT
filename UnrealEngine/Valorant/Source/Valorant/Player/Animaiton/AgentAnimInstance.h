// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State)
	bool bIsInAir = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State)
	bool bIsCrouch = false;
	
	virtual  void NativeUpdateAnimation(float DeltaSeconds) override;
};
