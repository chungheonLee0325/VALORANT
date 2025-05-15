// Fill out your copyright notice in the Description page of Project Settings.


#include "FireGround.h"


AFireGround::AFireGround()
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

void AFireGround::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, Instigator Name: %s"), __FUNCTION__, (GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL")));
	if (HasAuthority())
	{
		GroundMesh->OnComponentBeginOverlap.AddDynamic(this, &AFireGround::OnBeginOverlap);
		GroundMesh->OnComponentEndOverlap.AddDynamic(this, &AFireGround::OnEndOverlap);
	}
	GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this, &AFireGround::OnElapsedDuration, Duration, true);
}

void AFireGround::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, OtherActor Name: %s"), __FUNCTION__, *OtherActor->GetName());
	OverlappedActors.Add(OtherActor);
	if (!GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, &AFireGround::ApplyDamage, DamageRate, true);
	}
}

void AFireGround::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, OtherActor Name: %s"), __FUNCTION__, *OtherActor->GetName());
	OverlappedActors.Remove(OtherActor);
	if (OverlappedActors.Num() <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	}
}

void AFireGround::ApplyDamage()
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	
	for (auto* Actor : OverlappedActors)
	{
		UE_LOG(LogTemp, Warning, TEXT("%hs Called, Actor Name: %s"), __FUNCTION__, *Actor->GetName());
	}
}

void AFireGround::OnElapsedDuration()
{
	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	Destroy();
}