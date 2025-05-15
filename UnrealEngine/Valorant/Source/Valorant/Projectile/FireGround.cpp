// Fill out your copyright notice in the Description page of Project Settings.


#include "FireGround.h"


// Sets default values
AFireGround::AFireGround()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	SetRootComponent(GroundMesh);
	GroundMesh->SetRelativeScale3D(FVector(8.f, 8.f, 0.2f));
	ConstructorHelpers::FObjectFinder<UStaticMesh> GroundMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cylinder.Cylinder'"));
	if (GroundMeshAsset.Succeeded())
	{
		GroundMesh->SetStaticMesh(GroundMeshAsset.Object);
	}
	ConstructorHelpers::FObjectFinder<UMaterialInstance> GroundMaterial(TEXT("/Script/Engine.MaterialInstanceConstant'/Engine/VREditor/UI/FrameMaterial.FrameMaterial'"));
	if (GroundMaterial.Succeeded())
	{
		GroundMesh->SetMaterial(0, GroundMaterial.Object);
	}
	GroundMesh->SetCollisionProfileName("OverlapOnlyPawn");
}

// Called when the game starts or when spawned
void AFireGround::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, Instigator Name: %s"), __FUNCTION__, (GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL")));
	GroundMesh->OnComponentBeginOverlap.AddDynamic(this, &AFireGround::OnBeginOverlap);
	GroundMesh->OnComponentEndOverlap.AddDynamic(this, &AFireGround::OnEndOverlap);
}

void AFireGround::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, OtherActor Name: %s"), __FUNCTION__, *OtherActor->GetName());
}

void AFireGround::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, OtherActor Name: %s"), __FUNCTION__, *OtherActor->GetName());
}

// Called every frame
void AFireGround::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

