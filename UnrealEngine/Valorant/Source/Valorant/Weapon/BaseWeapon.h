// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValorantObject/BaseInteractor.h"
#include "BaseWeapon.generated.h"

class UPickUpComponent;
class UInputMappingContext;
class UInputAction;
class ABaseAgent;
struct FGunRecoilData;
struct FWeaponData;

UCLASS(config=Game)
class VALORANT_API ABaseWeapon : public ABaseInteractor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta = (AllowPrivateAccess = "true"))
	int WeaponID = 13;

	FWeaponData* WeaponData = nullptr;
	TArray<FGunRecoilData> RecoilData;
	float LastFireTime = -9999.0f;
	// 발사/사용 주기 (1 / FireRate)
	float FireInterval = 0.08f;
	// 탄창 크기
	int MagazineSize = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UPickUpComponent> PickUpModule;

	UPROPERTY()
	TObjectPtr<ABaseAgent> Agent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* StartFireAction;

	/** EndFire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* EndFireAction;

	/** StartReload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* StartReloadAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* DropAction;

	bool bIsFiring = false;
	int RecoilLevel = 0;
	float TotalRecoilOffsetPitch = 0.0f;
	float TotalRecoilOffsetYaw = 0.0f;
	FTimerHandle AutoFireHandle;
	FTimerHandle ReloadHandle;

public:
	// 탄창 내 남은 탄약
	UPROPERTY(BlueprintReadOnly)
	int MagazineAmmo = 0;
	// 여분 탄약 (장전되어있는 탄창 내 탄약은 제외)
	UPROPERTY(BlueprintReadOnly)
	int SpareAmmo = 0;
	
	// Sets default values for this component's properties
	ABaseWeapon();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void AttachWeapon(ABaseAgent* PickUpAgent);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerRPC_Fire(const FVector& Location, const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void EndFire();
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopReload();

public:
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartReload();
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Drop();

};
