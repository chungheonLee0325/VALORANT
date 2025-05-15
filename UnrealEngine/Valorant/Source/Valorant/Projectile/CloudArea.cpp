// Fill out your copyright notice in the Description page of Project Settings.


#include "CloudArea.h"


ACloudArea::ACloudArea()
{
	PrimaryActorTick.bCanEverTick = false;

	AreaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	SetRootComponent(AreaMesh);
	const float Scale = Radius * 2.f / 100.f;
	AreaMesh->SetRelativeScale3D(FVector(Scale));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AreaMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (AreaMeshAsset.Succeeded())
	{
		AreaMesh->SetStaticMesh(AreaMeshAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterial> AreaMaterial(TEXT("/Script/Engine.Material'/Engine/MapTemplates/MASTER_Sky_Material.MASTER_Sky_Material'"));
	if (AreaMaterial.Succeeded())
	{
		AreaMesh->SetMaterial(0, AreaMaterial.Object);
	}
	AreaMesh->SetCollisionProfileName("NoCollision");
}

void ACloudArea::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, Instigator Name: %s"), __FUNCTION__, (GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL")));
	GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this, &ACloudArea::OnElapsedDuration, Duration, true);
}

void ACloudArea::OnElapsedDuration()
{
	Destroy();
}