// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceManager/ValorantGameType.h"
#include "ValorantObject/BaseInteractor.h"
#include "BaseWeapon.generated.h"

class UValorantPickUpComponent;
class UGameplayEffect;
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
	
	// 무기가 이전에 사용된 적이 있는지 (발사, 라운드 경험 등)
	// true인 경우 판매 불가, 땅에 드롭함
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon")
	bool bWasUsed = false;
	
	// Sets default values for this component's properties
	ABaseWeapon();

	// WeaponID 설정
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetWeaponID(int32 NewWeaponID);
	
	// WeaponID 반환
	UFUNCTION(BlueprintCallable, Category="Weapon")
	int32 GetWeaponID() const { return WeaponID; }
	
	// 무기 사용 여부 설정
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetWasUsed(bool bNewWasUsed);
	
	// 무기 사용 여부 반환
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool GetWasUsed() const { return bWasUsed; }

	// 무기 사용 여부에 따른 시각적 효과 업데이트
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void UpdateVisualState();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

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
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartReload();
	
	UFUNCTION(BlueprintCallable)
	EWeaponCategory GetWeaponCategory() { return WeaponData->WeaponCategory; }

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> NewDamageEffectClass;

	/*
	 *	PickUp & Drop 관련
	 */
public:
	virtual bool CanAutoPickUp(ABaseAgent* Agent) const override;
	virtual bool CanDrop() const override;
	virtual void PickUp(ABaseAgent* Agent) override;
	virtual void Drop() override;
	
	UFUNCTION()
	void AttachWeapon(ABaseAgent* PickUpAgent);

	// 무기 사용 여부 리셋 (라운드 시작 시)
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void ResetUsedStatus();

};
