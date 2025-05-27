// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharSelectCamera.generated.h"

class USpringArmComponent;
class ACharSelectCharacterActor;
class UCameraComponent;

UCLASS()
class VALORANT_API ACharSelectCamera : public AActor
{
	GENERATED_BODY()

public:
	ACharSelectCamera();
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<ACharSelectCharacterActor> CharacterActor = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> CameraMesh = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UCameraComponent> Camera = nullptr;
	
	UFUNCTION(BlueprintCallable)
	void OnSelectedAgent(const int SelectedAgentID);

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlaySelectAnimation(const UAnimMontage* Montage);
};
