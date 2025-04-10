// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAgent.h"


ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ThirdPersonMesh");
	ThirdPersonMesh->SetupAttachment(GetRootComponent());
	
	ThirdPersonMesh->AlwaysLoadOnClient = true;
	ThirdPersonMesh->AlwaysLoadOnServer = true;
	ThirdPersonMesh->bOwnerNoSee = false;
	ThirdPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	ThirdPersonMesh->bCastDynamicShadow = true;
	ThirdPersonMesh->bAffectDynamicIndirectLighting = true;
	ThirdPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ThirdPersonMesh->SetGenerateOverlapEvents(false);
	ThirdPersonMesh->SetCanEverAffectNavigation(false);
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled())
	{
		GetMesh()->SetOwnerNoSee(true);
		ThirdPersonMesh->SetOwnerNoSee(false);
	}
	else
	{
		GetMesh()->SetOwnerNoSee(false);
		ThirdPersonMesh->SetOwnerNoSee(true);
	}
}

void ABaseAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

