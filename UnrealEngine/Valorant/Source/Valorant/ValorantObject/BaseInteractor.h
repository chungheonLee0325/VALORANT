// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceManager/ValorantGameType.h"
#include "BaseInteractor.generated.h"

class UWidgetComponent;
class USphereComponent;
class ABaseAgent;

UCLASS()
class VALORANT_API ABaseInteractor : public AActor
{
	GENERATED_BODY()

public:
	ABaseInteractor();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> Sphere = nullptr;

	UPROPERTY(Replicated)
	TObjectPtr<ABaseAgent> OwnerAgent = nullptr;

	UPROPERTY()
	EInteractorType InteractorType = EInteractorType::None;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWidgetComponent* InteractWidget;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void ServerOnly_OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	void OnDetect(bool bIsDetect);
	// 현재 Agent가 이 Interactor를 자동으로 주울 수 있는지 여부
	virtual bool ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const;
	// 버릴 수 있는지 여부
	virtual bool ServerOnly_CanDrop() const;
	// Agent가 가까이 가서 바라보았을 때 상호작용 가능한지 여부
	virtual bool ServerOnly_CanInteract() const;
	UFUNCTION(Server, Reliable)
	virtual void ServerRPC_PickUp(ABaseAgent* Agent);
	UFUNCTION(Server, Reliable)
	virtual void ServerRPC_Drop();
	UFUNCTION(Server, Reliable)
	virtual void ServerRPC_Interact(ABaseAgent* InteractAgent);

	EInteractorType GetInteractorType() const { return InteractorType; }
};