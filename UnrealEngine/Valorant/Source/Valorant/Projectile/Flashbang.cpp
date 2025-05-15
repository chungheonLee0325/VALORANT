// Fill out your copyright notice in the Description page of Project Settings.


#include "Flashbang.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


AFlashbang::AFlashbang()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/KAYO_Ability_Flashbang/AB_Grenadier_S0_4_Skelmesh.AB_Grenadier_S0_4_Skelmesh'"));
	if (MeshAsset.Succeeded())
	{
		Mesh->SetSkeletalMesh(MeshAsset.Object);
	}
	
	Sphere->SetSphereRadius(20.0f);
	
	ProjectileMovement->bAutoActivate = bAutoActivate;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = Friction;
	ExplosionTime = MaximumAirTime;
}

void AFlashbang::ActiveProjectileMovement(const bool bAltFire)
{
	FVector LocalDir = bAltFire ? FVector(0.7f, 0.f, 0.3f) : FVector(1.f, 0.f, 0.f);
	FVector WorldDir = GetActorRotation().RotateVector(LocalDir).GetSafeNormal();
	float SpeedToUse = bAltFire ? SpeedAltFire : Speed;
	ProjectileMovement->Velocity = WorldDir * SpeedToUse;
	ProjectileMovement->InitialSpeed = bAltFire ? SpeedAltFire : Speed;
	ProjectileMovement->MaxSpeed = bAltFire ? SpeedAltFire : Speed;
	ExplosionTime = bAltFire ? MaximumAirTimeAltFire : MaximumAirTime;
	ProjectileMovement->Activate();
}

void AFlashbang::BeginPlay()
{
	Super::BeginPlay();
}

void AFlashbang::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ProjectileMovement->IsActive())
	{
		FRotator RotationPerTick(240.f * DeltaSeconds, 540.f * DeltaSeconds, 480.f * DeltaSeconds);
		Mesh->AddLocalRotation(RotationPerTick);
	}
	
	if (ProjectileMovement->IsActive() && false == bElapsedAirTime)
	{
		CurrentAirTime += DeltaSeconds;
		if (CurrentAirTime >= ExplosionTime)
		{
			OnElapsedMaxAirTime();
		}
	}
}

void AFlashbang::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	ExplosionTime = FMath::Min(ExplosionTime, AirTimeOnBounce);
}

void AFlashbang::OnElapsedMaxAirTime()
{
	UE_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	bElapsedAirTime = true;
	Destroy();
}