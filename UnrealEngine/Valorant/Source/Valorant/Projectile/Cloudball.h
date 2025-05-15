// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseProjectile.h"
#include "Cloudball.generated.h"

class ACloudArea;

UCLASS()
class VALORANT_API ACloudball : public ABaseProjectile
{
	GENERATED_BODY()

public:
	ACloudball();

private:
	// Ref: https://valorant.fandom.com/wiki/Deployment_types#Projectile
	const float Speed = 2900;
	const float Gravity = 0.45f;
	const bool bShouldBounce = true;
	// const float Bounciness = 0.2f;
	// const float Friction = 0.8f;
	// const float EquipTime = 0.8f;
	const float UnequipTime = 0.25f;
	const float MaximumAirTime = 2.0f;
	FTimerHandle AirTimeHandle;

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<ACloudArea> CloudAreaClass = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;

	void OnElapsedMaxAirTime();
	void SpawnCloudArea(const FVector& SpawnPoint) const;
};