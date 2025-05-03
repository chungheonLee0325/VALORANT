// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ValorantObject/BaseInteractor.h"
#include "Spike.generated.h"

UCLASS()
class VALORANT_API ASpike : public ABaseInteractor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASpike();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
