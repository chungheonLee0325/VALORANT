// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "StimBeaconAnim.generated.h"

class AStimBeacon;
enum class EStimBeaconState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutroEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeployEnded);

/**
 * 
 */
UCLASS()
class VALORANT_API UStimBeaconAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<AStimBeacon> Owner;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EStimBeaconState State;
	UPROPERTY()
	FOnOutroEnded OnDeployEnded;
	UPROPERTY()
	FOnOutroEnded OnOutroEnded;
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeploy();

protected:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	UFUNCTION()
	void AnimNotify_OnDeployEnded() const;
	UFUNCTION()
	void AnimNotify_OnOutroEnded() const;
};
