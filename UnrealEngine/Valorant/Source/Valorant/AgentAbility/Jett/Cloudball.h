// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
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
	const float Radius = 335;
	const float Duration = 2.5f;
	FTimerHandle DurationTimerHandle;

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> InnerMesh = nullptr;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> OuterMesh = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;

	void OnElapsedMaxAirTime();
	void ActiveCloudArea(const FVector& SpawnPoint);
	void OnElapsedDuration();
};