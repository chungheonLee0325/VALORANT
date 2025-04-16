// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "ResourceManager/ValorantGameType.h"
#include "ValorantWeaponComponent.generated.h"

class AValorantCharacter;

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
	UPROPERTY(EditDefaultsOnly, Category=Weapon)
	int WeaponID = 1;
	
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AValorantProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* StartFireAction;

	/** EndFire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* EndFireAction;

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool AttachWeapon(AValorantCharacter* TargetCharacter);

private:
	/** The Character holding this weapon*/
	AValorantCharacter* Character;

	FWeaponData* WeaponData = nullptr;
	TArray<FGunRecoilData> RecoilData;
	// 발사/사용 주기 (1 / FireRate)
	float FireInterval = 0.08f;
	
	int RecoilLevel = 0;
	float LastFireTime = -9999.0f;
	FTimerHandle AutoFireHandle;
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();
	
	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void EndFire();
};
