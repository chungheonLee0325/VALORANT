// Fill out your copyright notice in the Description page of Project Settings.


#include "Spike.h"


ASpike::ASpike()
{
	PrimaryActorTick.bCanEverTick = true;
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh> SpikeMeshObj(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Spike/Spike.Spike'"));
	if (SpikeMeshObj.Succeeded())
	{
		Mesh->SetSkeletalMesh(SpikeMeshObj.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.34f));
}

void ASpike::BeginPlay()
{
	Super::BeginPlay();
}

void ASpike::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}