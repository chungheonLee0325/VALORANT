// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireGround.generated.h"

UCLASS()
class VALORANT_API AFireGround : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFireGround();
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundMesh = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
