// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeKnife.h"


// Sets default values
AMeleeKnife::AMeleeKnife()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMeleeKnife::BeginPlay()
{
	Super::BeginPlay();
}

void AMeleeKnife::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AMeleeKnife::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	return false;
}

bool AMeleeKnife::ServerOnly_CanDrop() const
{
	return false;
}

