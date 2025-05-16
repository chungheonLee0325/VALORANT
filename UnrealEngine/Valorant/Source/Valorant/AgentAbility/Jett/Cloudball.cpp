// Fill out your copyright notice in the Description page of Project Settings.


#include "Cloudball.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


ACloudball::ACloudball()
{
	PrimaryActorTick.bCanEverTick = false;

	InnerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerMesh"));
	InnerMesh->SetupAttachment(GetRootComponent());
	InnerMesh->SetRelativeScale3D(FVector(.3f));
	
	OuterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterMesh"));
	OuterMesh->SetupAttachment(GetRootComponent());
	OuterMesh->SetRelativeScale3D(FVector(.3f));

	Sphere->SetSphereRadius(15.0f);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CloudballMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (CloudballMeshAsset.Succeeded())
	{
		InnerMesh->SetStaticMesh(CloudballMeshAsset.Object);
		OuterMesh->SetStaticMesh(CloudballMeshAsset.Object);
	}
	
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> InnerMaterial(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Resource/Props/Projectiles/Jett_Ability_Windball/MI_InnerWind.MI_InnerWind'"));
	if (InnerMaterial.Succeeded())
	{
		InnerMesh->SetMaterial(0, InnerMaterial.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> OuterMaterial(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Resource/Props/Projectiles/Jett_Ability_Windball/MI_OuterWind.MI_OuterWind'"));
	if (OuterMaterial.Succeeded())
	{
		OuterMesh->SetMaterial(0, OuterMaterial.Object);
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
	ActiveCloudArea(ImpactResult.ImpactPoint);
}

void ACloudball::OnElapsedMaxAirTime()
{
	ActiveCloudArea(GetActorLocation());
}

void ACloudball::ActiveCloudArea(const FVector& SpawnPoint)
{
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetActive(false);
	if (GetWorld()->GetTimerManager().IsTimerActive(DurationTimerHandle) == false)
	{
		Sphere->SetSphereRadius(Radius);
		const float Scale = Radius / 100.f;
		InnerMesh->SetRelativeScale3D(FVector(Scale));
		OuterMesh->SetRelativeScale3D(FVector(Scale));
		GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this, &ACloudball::OnElapsedDuration, Duration, false);
	}
}

void ACloudball::OnElapsedDuration()
{
	Destroy();
}