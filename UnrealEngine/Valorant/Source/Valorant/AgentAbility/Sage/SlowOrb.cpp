// Fill out your copyright notice in the Description page of Project Settings.


#include "SlowOrb.h"

#include "AgentAbility/BaseGround.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


// Sets default values
ASlowOrb::ASlowOrb()
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

// Called when the game starts or when spawned
void ASlowOrb::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(AirTimeHandle, this, &ASlowOrb::OnElapsedMaxAirTime, MaximumAirTime, false);
}

void ASlowOrb::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	if (ImpactResult.ImpactNormal.Z > 0.5f)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Instigator = this->GetInstigator();
		GetWorld()->SpawnActor<ABaseGround>(BaseGroundClass, ImpactResult.ImpactPoint, FRotator::ZeroRotator, SpawnParameters);
		Destroy();
	}
}

void ASlowOrb::OnElapsedMaxAirTime()
{
	ProjectileMovement->Velocity = FVector(0, 0, -Speed);
}
