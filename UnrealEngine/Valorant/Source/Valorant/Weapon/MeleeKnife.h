// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseWeapon.h"
#include "MeleeKnife.generated.h"

UCLASS()
class VALORANT_API AMeleeKnife : public ABaseWeapon
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMeleeKnife();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual bool ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const override;
	virtual bool ServerOnly_CanDrop() const override;
};
