// Fill out your copyright notice in the Description page of Project Settings.


#include "KayoGrenade.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


AKayoGrenade::AKayoGrenade()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	
	Sphere->SetSphereRadius(15.0f);
	Mesh->SetRelativeScale3D(FVector(0.15f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GrenadeMeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/Resource/Props/Projectiles/KAYO_Ability_Grenade/KayoGrenade.KayoGrenade'"));
	if (GrenadeMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(GrenadeMeshAsset.Object);
	}
	
	static ConstructorHelpers::FObjectFinder<UMaterial> GrenadeMaterial(TEXT("/Script/Engine.Material'/Game/Resource/Props/Projectiles/KAYO_Ability_Grenade/M_KayoGrenade.M_KayoGrenade'"));
	if (GrenadeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, GrenadeMaterial.Object);
	}

	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = Friction;
}

void AKayoGrenade::BeginPlay()
{
	Super::BeginPlay();
	
}

void AKayoGrenade::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, ImpactNormal: %s"), __FUNCTION__, *ImpactResult.ImpactNormal.ToString());
	if (ImpactResult.ImpactNormal.Z > 0.8f)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(DeterrentTimerHandle) == false)
		{
			GetWorld()->GetTimerManager().SetTimer(DeterrentTimerHandle, this, &AKayoGrenade::ActiveDeterrent, DeterrentInterval, true, ActiveTime);
			ProjectileMovement->StopMovementImmediately();
			ProjectileMovement->SetActive(false);
		}
	}
}

void AKayoGrenade::ActiveDeterrent()
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	--DeterrentCount;
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, Remain %d"), __FUNCTION__, DeterrentCount);
	if (DeterrentCount <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(DeterrentTimerHandle);
		Destroy();
	}
}
