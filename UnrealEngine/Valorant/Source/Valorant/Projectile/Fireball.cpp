// Fill out your copyright notice in the Description page of Project Settings.


#include "Fireball.h"

#include "FireGround.h"


// Sets default values
AFireball::AFireball()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AFireball::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFireball::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFireball::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	if (ImpactResult.ImpactNormal.Z > 0.5f)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Instigator = this->GetInstigator();
		GetWorld()->SpawnActor<AFireGround>(FireGroundClass, ImpactResult.ImpactPoint, FRotator::ZeroRotator, SpawnParameters);
		Destroy();
	}
}