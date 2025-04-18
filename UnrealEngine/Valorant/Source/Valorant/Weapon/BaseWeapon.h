// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseWeapon.generated.h"

struct FGunRecoilData;
struct FWeaponData;

UCLASS(config=Game)
class VALORANT_API ABaseWeapon : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta = (AllowPrivateAccess = "true"))
	int WeaponID = 12;

	FWeaponData* WeaponData = nullptr;
	TArray<FGunRecoilData> RecoilData;
	int RecoilLevel = 0;
	float LastFireTime = -9999.0f;
	// 발사/사용 주기 (1 / FireRate)
	float FireInterval = 0.08f;
	// 탄창 크기
	int MagazineSize = 0;
	// 탄창 내 남은 탄약
	int MagazineAmmo = 0;
	// 여분 탄약 (장전되어있는 탄창 내 탄약은 제외)
	int SpareAmmo = 0;
	
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// UPROPERTY()
	// TObjectPtr<UWidgetComponent> PickUpUIComponent;

	// UPROPERTY()
	// TObjectPtr<USphereComponent> PickUpComponent;

public:
	// Sets default values for this component's properties
	ABaseWeapon();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;
};
