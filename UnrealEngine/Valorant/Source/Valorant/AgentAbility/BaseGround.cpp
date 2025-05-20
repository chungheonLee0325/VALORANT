// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGround.h"

#include "Player/Agent/BaseAgent.h"


// Sets default values
ABaseGround::ABaseGround()
{
	PrimaryActorTick.bCanEverTick = false;

	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	SetRootComponent(GroundMesh);
	const float Scale = Radius * 2.f / 100.f;
	GroundMesh->SetRelativeScale3D(FVector(Scale, Scale, 0.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cylinder.Cylinder'"));
	if (GroundMeshAsset.Succeeded())
	{
		GroundMesh->SetStaticMesh(GroundMeshAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> GroundMaterial(TEXT("/Script/Engine.MaterialInstanceConstant'/Engine/VREditor/LaserPointer/TranslucentLaserPointerMaterialInst.TranslucentLaserPointerMaterialInst'"));
	if (GroundMaterial.Succeeded())
	{
		GroundMesh->SetMaterial(0, GroundMaterial.Object);
	}
	GroundMesh->SetCollisionProfileName("OverlapOnlyPawn");
}

// Called when the game starts or when spawned
void ABaseGround::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, Instigator Name: %s"), __FUNCTION__, (GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL")));
	if (HasAuthority())
	{
		GroundMesh->OnComponentBeginOverlap.AddDynamic(this, &ABaseGround::OnBeginOverlap);
		GroundMesh->OnComponentEndOverlap.AddDynamic(this, &ABaseGround::OnEndOverlap);
	}
	GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this, &ABaseGround::OnElapsedDuration, Duration, true);

	
}

void ABaseGround::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		OverlappedAgents.Add(Agent);
	}
	if (!GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, &ABaseGround::ApplyGameEffect, DamageRate, true);
	}
}

void ABaseGround::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	
	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		OverlappedAgents.Remove(Agent);
	}
	
	if (OverlappedAgents.Num() <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	}
}

void ABaseGround::ApplyGameEffect()
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	
	for (auto* Agent : OverlappedAgents)
	{
		Agent->ServerApplyGE(GameplayEffect);
	}
}

void ABaseGround::OnElapsedDuration()
{
	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	Destroy();
}
