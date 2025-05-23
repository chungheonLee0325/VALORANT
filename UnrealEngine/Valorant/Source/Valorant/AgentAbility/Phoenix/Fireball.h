// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "Fireball.generated.h"

class ABaseGround;
class AFireGround;

UCLASS()
class VALORANT_API AFireball : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AFireball();

private:
	// Ref: https://valorant.fandom.com/wiki/Deployment_types#Projectile
	const float Speed = 1800;
	const float Gravity = 0.3f;
	const bool bShouldBounce = true;
	const float Bounciness = 0.2f;
	const float Friction = 0.8f;
	const float EquipTime = 0.8f;
	const float UnequipTime = 0.7f;
	const float MaximumAirTime = 1.5f;
	FTimerHandle AirTimeHandle;
	
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
	UPROPERTY(EditAnywhere)
	TSubclassOf<ABaseGround> FireGroundClass = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	
	void OnElapsedMaxAirTime();
};