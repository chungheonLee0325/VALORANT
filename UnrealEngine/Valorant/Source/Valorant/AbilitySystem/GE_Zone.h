// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GE_Zone.generated.h"

class UBoxComponent;
class UGameplayEffect;

UCLASS()
class VALORANT_API AGE_Zone : public AActor
{
	GENERATED_BODY()

public:
	AGE_Zone();
	UPROPERTY(EditDefaultsOnly)
	UBoxComponent* BoxComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GAS)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite)
	class UTextRenderComponent* Text;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite)
	UTextRenderComponent* NameText;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnActorxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	virtual void Tick(float DeltaTime) override;
};
