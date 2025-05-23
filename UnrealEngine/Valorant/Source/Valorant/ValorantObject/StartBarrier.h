// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StartBarrier.generated.h"

UCLASS()
class VALORANT_API AStartBarrier : public AActor
{
	GENERATED_BODY()

public:
	AStartBarrier();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void DeactiveBarrier();
	UFUNCTION()
	void ActiveBarrier();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ActiveBarrier();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DeactiveBarrier();

public:
	virtual void Tick(float DeltaTime) override;
};
