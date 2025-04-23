// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseInteractor.h"


ABaseInteractor::ABaseInteractor()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractWidget = CreateDefaultSubobject<UWidgetComponent>("InteractWidget");
	InteractWidget->SetVisibility(false);
}

void ABaseInteractor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseInteractor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseInteractor::InteractActive(bool bIsActive)
{
	InteractWidget->SetVisibility(bIsActive);
}

