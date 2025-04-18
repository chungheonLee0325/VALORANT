// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "PickUpComponent.generated.h"

class ABaseAgent;
class UCapsuleComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUpWeapon, ABaseAgent*, PickUpAgent);

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VALORANT_API UPickUpComponent : public UCapsuleComponent
{
	GENERATED_BODY()

	bool bIsEnableBeginOverlap = false;
	
public:
	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnPickUpWeapon OnPickUp;
	
	// Sets default values for this actor's properties
	UPickUpComponent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	/** Code for when something overlaps this component */
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	void PickUp(ABaseAgent* Character);
	void SetEnableBeginOverlap();
};