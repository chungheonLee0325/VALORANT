// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseGround.generated.h"

class UGameplayEffect;
class ABaseAgent;

UCLASS()
class VALORANT_API ABaseGround : public AActor
{
	GENERATED_BODY()

public:
	ABaseGround();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TSet<ABaseAgent*> OverlappedAgents;
	
	FTimerHandle DamageTimerHandle;
	FTimerHandle DurationTimerHandle;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyGameEffect();

	void OnElapsedDuration();

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

private:
	const float Radius = 450;
	const float Duration = 4;
	const float DamageRate = 0.0167f;
};
