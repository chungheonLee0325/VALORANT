// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "BaseInteractor.generated.h"

class USphereComponent;
class UValorantPickUpComponent;
class ABaseAgent;

UCLASS()
class VALORANT_API ABaseInteractor : public AActor
{
	GENERATED_BODY()

public:
	ABaseInteractor();

protected:
	UPROPERTY()
	TObjectPtr<USphereComponent> Sphere = nullptr;

	UPROPERTY()
	TObjectPtr<ABaseAgent> OwnerAgent = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* InteractWidget;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	void InteractActive(bool bIsActive);
	// 현재 Agent가 이 Interactor를 자동으로 주울 수 있는지 여부
	virtual bool CanAutoPickUp(ABaseAgent* Agent) const;
	// 버릴 수 있는지 여부
	virtual bool CanDrop() const;
	virtual void PickUp(ABaseAgent* Agent);
	virtual void Drop();
};