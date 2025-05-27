// Fill out your copyright notice in the Description page of Project Settings.


#include "Fireball.h"

#include "FireGround.h"
#include "AgentAbility/BaseGround.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


AFireball::AFireball()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	
	Sphere->SetSphereRadius(15.0f);
	Mesh->SetRelativeScale3D(FVector(.3f));
	
	static ConstructorHelpers::FObjectFinder<UMaterial> FireballMaterial(TEXT("/Script/Engine.Material'/Engine/VREditor/LaserPointer/LaserPointerMaterial.LaserPointerMaterial'"));
	if (FireballMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, FireballMaterial.Object);
	}
	
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = Friction;
}

void AFireball::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(AirTimeHandle, this, &AFireball::OnElapsedMaxAirTime, MaximumAirTime, false);
}

void AFireball::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	if (ImpactResult.ImpactNormal.Z > 0.5f)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Instigator = this->GetInstigator();
		GetWorld()->SpawnActor<ABaseGround>(FireGroundClass, ImpactResult.ImpactPoint, FRotator::ZeroRotator, SpawnParameters);
		Destroy();
	}
}

void AFireball::OnElapsedMaxAirTime()
{
	ProjectileMovement->Velocity = FVector(0, 0, -Speed);
}