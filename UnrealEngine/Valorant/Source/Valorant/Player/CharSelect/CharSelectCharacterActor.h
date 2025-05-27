// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharSelectCharacterActor.generated.h"

UCLASS()
class VALORANT_API ACharSelectCharacterActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACharSelectCharacterActor();
	UFUNCTION(BlueprintImplementableEvent)
	void PlaySelectAnimation(const UAnimMontage* Montage);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
