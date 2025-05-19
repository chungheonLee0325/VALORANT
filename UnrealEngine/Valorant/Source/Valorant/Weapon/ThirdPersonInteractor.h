// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThirdPersonInteractor.generated.h"

UCLASS()
class VALORANT_API AThirdPersonInteractor : public AActor
{
	GENERATED_BODY()

public:
	AThirdPersonInteractor();
	
public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_InitWeapon(const int WeaponId);
};