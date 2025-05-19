// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "IncendiaryBomb.generated.h"

UCLASS()
class VALORANT_API AIncendiaryBomb : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AIncendiaryBomb();

private:
	const float Speed = 1800;
	const float Gravity = 0.45f;
	const bool bShouldBounce = true;
	const float Bounciness = 0.5f;
	const bool bBounceAngleAffectsFriction = true;
	const float Friction = 0.7f;
	const float MinFrictionFraction = 0.6f;

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
};