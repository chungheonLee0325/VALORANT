// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CloudArea.generated.h"

UCLASS()
class VALORANT_API ACloudArea : public AActor
{
	GENERATED_BODY()

public:
	ACloudArea();

private:
	const float Radius = 335;
	const float Duration = 2.5f;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UStaticMeshComponent> AreaMesh = nullptr;

	FTimerHandle DurationTimerHandle;

protected:
	virtual void BeginPlay() override;

	void OnElapsedDuration();
};