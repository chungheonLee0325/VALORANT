// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseInteractor.generated.h"

enum class EInteractorType : uint8;
class AThirdPersonInteractor;
class UWidgetComponent;
class USphereComponent;
class ABaseAgent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractorDrop);

UCLASS()
class VALORANT_API ABaseInteractor : public AActor
{
	GENERATED_BODY()

public:
	ABaseInteractor();
	
protected:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess = "true"), Replicated)
	TObjectPtr<AThirdPersonInteractor> ThirdPersonInteractor = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> Sphere = nullptr;

	UPROPERTY(Replicated, ReplicatedUsing=OnRep_OwnerAgent, VisibleAnywhere)
	TObjectPtr<ABaseAgent> OwnerAgent = nullptr;
	UFUNCTION()
	void OnRep_OwnerAgent();
	void SetOwnerAgent(ABaseAgent* NewAgent);

	UPROPERTY()
	EInteractorType InteractorType;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UWidgetComponent> DetectWidgetComponent = nullptr;
	ABaseAgent* GetOwnerAgent() const { return OwnerAgent; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginDestroy() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void ServerOnly_OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	bool HasOwnerAgent() const { return OwnerAgent != nullptr; }
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

	USkeletalMeshComponent* GetMesh() const { return Mesh; }
	EInteractorType GetInteractorType() const { return InteractorType; }

	void SetActive(bool bActive);
	UFUNCTION(Server, Reliable)
	void ServerRPC_SetActive(bool bActive);
	UFUNCTION(NetMulticast, Reliable)
	virtual void Multicast_SetActive(bool bActive);

	UPROPERTY(BlueprintAssignable)
	FOnEquip OnEquip;
	UPROPERTY(BlueprintAssignable)
	FOnFire OnFire;
	UPROPERTY(BlueprintAssignable)
	FOnReload OnReload;
	UPROPERTY(BlueprintAssignable)
	FOnReload OnPickUp;
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_BroadcastOnPickUp();
	UPROPERTY(BlueprintAssignable)
	FOnInteractorDrop OnInteractorDrop;
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_BroadcastOnDrop();

	virtual void PlayEquipAnimation();

	USkeletalMeshComponent* GetMesh() { return Mesh; }
};