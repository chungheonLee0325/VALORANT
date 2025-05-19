// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "StimBeacon.generated.h"

class UStimBeaconAnim;

UENUM(BlueprintType)
enum class EStimBeaconState : uint8
{
	ESBS_Idle,
	ESBS_Active,
	ESBS_Outtro
};

UCLASS()
class VALORANT_API AStimBeacon : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AStimBeacon();

private:
	const float Speed = 1650;
	const float Gravity = 0.7f;
	const bool bShouldBounce = true;
	const float Bounciness = 0.5f;
	const float UnequipTime = 0.7f;
	const float Radius = 600.0f;
	// +10% Equip speed
	// +15% Fire rate
	// +10% Reload speed
	// +10% Recovery speed
	// +15% Speed Boost
	const float BuffDuration = 4.0f;
	
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	EStimBeaconState State = EStimBeaconState::ESBS_Idle;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStimBeaconAnim> AnimInstance = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	UFUNCTION()
	void OnOutroAnimationEnded();
	UFUNCTION()
	void OnDeployAnimationEnded();
};
