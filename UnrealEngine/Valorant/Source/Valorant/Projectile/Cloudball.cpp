// Fill out your copyright notice in the Description page of Project Settings.


#include "Cloudball.h"

#include "CloudArea.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


ACloudball::ACloudball()
{
	PrimaryActorTick.bCanEverTick = false;
	
	Sphere->SetSphereRadius(15.0f);
	Mesh->SetRelativeScale3D(FVector(.3f));
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CloudballMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (CloudballMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(CloudballMeshAsset.Object);
	}
	
	static ConstructorHelpers::FObjectFinder<UMaterial> CloudballMaterial(TEXT("/Script/Engine.Material'/Engine/MapTemplates/MASTER_Sky_Material.MASTER_Sky_Material'"));
	if (CloudballMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, CloudballMaterial.Object);
	}
	
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	// ProjectileMovement->Bounciness = Bounciness;
	// ProjectileMovement->Friction = Friction;
}

void ACloudball::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(AirTimeHandle, this, &ACloudball::OnElapsedMaxAirTime, MaximumAirTime, false);
}

void ACloudball::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	SpawnCloudArea(ImpactResult.ImpactPoint);
	Destroy();
}

void ACloudball::OnElapsedMaxAirTime()
{
	SpawnCloudArea(GetActorLocation());
	Destroy();
}

void ACloudball::SpawnCloudArea(const FVector& SpawnPoint) const
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Instigator = this->GetInstigator();
	GetWorld()->SpawnActor<ACloudArea>(CloudAreaClass, SpawnPoint, FRotator::ZeroRotator, SpawnParameters);
}
