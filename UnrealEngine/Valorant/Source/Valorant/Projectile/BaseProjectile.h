// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class VALORANT_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABaseProjectile();
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere = nullptr;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
};
