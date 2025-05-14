// Fill out your copyright notice in the Description page of Project Settings.


#include "MapTestAgent.h"

#include "Kismet/GameplayStatics.h"


// Sets default values
AMapTestAgent::AMapTestAgent()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void AMapTestAgent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMapTestAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AMapTestAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}




