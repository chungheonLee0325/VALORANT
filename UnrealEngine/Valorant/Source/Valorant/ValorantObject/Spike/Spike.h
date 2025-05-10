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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	virtual void ServerRPC_PickUp(ABaseAgent* Agent) override;
	virtual void ServerRPC_Drop() override;
	virtual void ServerRPC_Interact(ABaseAgent* InteractAgent) override;

protected:
	virtual bool ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const override;
	virtual bool ServerOnly_CanDrop() const override;
	virtual bool ServerOnly_CanInteract() const override;
};
