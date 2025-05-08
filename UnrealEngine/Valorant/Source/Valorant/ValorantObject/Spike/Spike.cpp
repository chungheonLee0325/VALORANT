// Fill out your copyright notice in the Description page of Project Settings.


#include "Spike.h"

#include "Components/SphereComponent.h"


ASpike::ASpike()
{
	PrimaryActorTick.bCanEverTick = true;

	SpikeMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SpikeMesh"));
	ConstructorHelpers::FObjectFinder<USkeletalMesh> SpikeMeshObj(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Spike/Spike.Spike'"));
	if (SpikeMeshObj.Succeeded())
	{
		SpikeMesh->SetSkeletalMesh(SpikeMeshObj.Object);
	}
	SetRootComponent(SpikeMesh);
	
	InteractWidget->SetupAttachment(GetRootComponent());
	Sphere->SetupAttachment(RootComponent);
}

void ASpike::BeginPlay()
{
	Super::BeginPlay();
}

void ASpike::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}