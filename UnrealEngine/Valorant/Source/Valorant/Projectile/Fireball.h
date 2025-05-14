// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseProjectile.h"
#include "Fireball.generated.h"

class AFireGround;

UCLASS()
class VALORANT_API AFireball : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AFireball();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AFireGround> FireGroundClass = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
};
