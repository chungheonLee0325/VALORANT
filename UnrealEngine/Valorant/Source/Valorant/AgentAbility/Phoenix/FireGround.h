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
	AFireGround();

private:
	const float Radius = 450;
	const float Duration = 4;
	const float DamageRate = 0.0167f;
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TSet<AActor*> OverlappedActors;
	
	FTimerHandle DamageTimerHandle;
	FTimerHandle DurationTimerHandle;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyDamage();

	void OnElapsedDuration();
};