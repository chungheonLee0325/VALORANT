// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GA_Maker.generated.h"

class UAttributeSet;
class UGameplayAbility;
class UBoxComponent;

UCLASS()
class VALORANT_API AGA_Maker : public AActor
{
	GENERATED_BODY()

public:
	AGA_Maker();
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UBoxComponent* Box;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite)
	class UWidgetComponent* Widget;


protected:
	UFUNCTION()
	void OnActorxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	virtual void BeginPlay() override;

	UPROPERTY()
	TSubclassOf<UAttributeSet> AttributeSetClass;

public:
	virtual void Tick(float DeltaTime) override;
};
