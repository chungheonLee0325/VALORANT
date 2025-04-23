// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "BaseInteractor.generated.h"

UCLASS()
class VALORANT_API ABaseInteractor : public AActor
{
	GENERATED_BODY()

public:
	ABaseInteractor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* InteractWidget;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	void InteractActive(bool bIsActive);
};
