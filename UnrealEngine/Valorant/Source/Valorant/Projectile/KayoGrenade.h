// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseProjectile.h"
#include "KayoGrenade.generated.h"

UCLASS()
class VALORANT_API AKayoGrenade : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AKayoGrenade();

private:
	const float Speed = 1800;
	const float Gravity = 0.3f;
	const bool bShouldBounce = true;
	const float Bounciness = 0.2f;
	const float Friction = 0.8f;
	const float EquipTime = 0.7f;
	const float UnequipTime = 0.6f;
	const float ActiveTime = 0.5f;
	const float InnerRadius = 1000;
	const float OuterRadius = 4000;
	int DeterrentCount = 4;
	const float DeterrentInterval = 1.0f;
	const float MinDamage = 25;
	const float MaxDamage = 60;
	FTimerHandle DeterrentTimerHandle;

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	void ActiveDeterrent();
};