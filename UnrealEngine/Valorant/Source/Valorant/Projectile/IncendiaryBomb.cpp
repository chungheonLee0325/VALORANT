// Fill out your copyright notice in the Description page of Project Settings.


#include "IncendiaryBomb.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


AIncendiaryBomb::AIncendiaryBomb()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	Sphere->SetSphereRadius(7.0f);
	Mesh->SetRelativeScale3D(FVector(0.34f));
	Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BombMeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/Resource/Props/Projectiles/Brimstone_Ability_Incendiary/AB_Sarge_S0_Q_Shell_Skelmesh.AB_Sarge_S0_Q_Shell_Skelmesh'"));
	if (BombMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(BombMeshAsset.Object);
	}

	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->bBounceAngleAffectsFriction = bBounceAngleAffectsFriction;
	ProjectileMovement->Friction = Friction;
	ProjectileMovement->MinFrictionFraction = MinFrictionFraction;
}

void AIncendiaryBomb::BeginPlay()
{
	Super::BeginPlay();
	
}

void AIncendiaryBomb::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	if (IsActorBeingDestroyed())
	{
		return;
	}

	if (ImpactResult.ImpactNormal.Z > 0.5f && ImpactVelocity.Size() < 500.f)
	{
		Destroy();
	}
}
