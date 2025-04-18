// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "ResourceManager/ValorantGameType.h"
#include "ValorantWeaponComponent.generated.h"

class AValorantCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropWeapon, AValorantCharacter*, Character);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VALORANT_API UValorantWeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	UValorantWeaponComponent();
	
protected:
	virtual void BeginPlay() override;
	
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	UPROPERTY(EditAnywhere, Category=Weapon)
	int WeaponID = 13;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* StartFireAction;

	/** EndFire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* EndFireAction;

	/** StartReload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	const UInputAction* StartReloadAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	const UInputAction* DropAction;
	
	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool AttachWeapon(AValorantCharacter* TargetCharacter);

private:
	/** The Character holding this weapon*/
	AValorantCharacter* Character;
	
	FWeaponData* WeaponData = nullptr;
	
	TArray<FGunRecoilData> RecoilData;
	int RecoilLevel = 0;
	
	float LastFireTime = -9999.0f;
	// 발사/사용 주기 (1 / FireRate)
	float FireInterval = 0.08f;
	FTimerHandle AutoFireHandle;
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void EndFire();

	FTimerHandle ReloadHandle;
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartReload();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopReload();

public:
	// 탄창 크기
	int MagazineSize = 0;
	// 탄창 내 남은 탄약
	UPROPERTY(BlueprintReadOnly)
	int MagazineAmmo = 0;
	// 여분 탄약 (장전되어있는 탄창 내 탄약은 제외)
	UPROPERTY(BlueprintReadOnly)
	int SpareAmmo = 0;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Drop();

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnDropWeapon OnDropWeapon;
};
