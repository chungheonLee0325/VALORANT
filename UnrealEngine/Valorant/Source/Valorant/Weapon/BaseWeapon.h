// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceManager/ValorantGameType.h"
#include "ValorantObject/BaseInteractor.h"
#include "BaseWeapon.generated.h"

class UGameplayEffect;
struct FGunRecoilData;
struct FWeaponData;
class UInputMappingContext;
class UInputAction;

UCLASS(config=Game)
class VALORANT_API ABaseWeapon : public ABaseInteractor
{
	GENERATED_BODY()

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category="Weapon", meta = (AllowPrivateAccess = "true"))
	int WeaponID = 0;

	FWeaponData* WeaponData = nullptr;
	TArray<FGunRecoilData> RecoilData;
	float LastFireTime = -9999.0f;
	// 발사/사용 주기 (1 / FireRate)
	float FireInterval = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* FireMappingContext = nullptr;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* StartFireAction = nullptr;

	/** EndFire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* EndFireAction = nullptr;

	/** StartReload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* StartReloadAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* DropAction = nullptr;

protected:
	// 탄창 크기
	int MagazineSize = 0;
	
	bool bIsFiring = false;
	UPROPERTY(Replicated)
	bool bIsReloading = false;
	int RecoilLevel = 0;
	float TotalRecoilOffsetPitch = 0.0f;
	float TotalRecoilOffsetYaw = 0.0f;
	FTimerHandle AutoFireHandle;
	FTimerHandle ReloadHandle;
	FTimerHandle RecoverRecoilLevelHandle;

	UPROPERTY()
	UAnimMontage* AM_Fire;
	UPROPERTY()
	UAnimMontage* AM_Reload;

public:
	// 탄창 내 남은 탄약
	UPROPERTY(ReplicatedUsing=OnRep_Ammo, BlueprintReadOnly)
	int MagazineAmmo = 0;
	// 여분 탄약 (장전되어있는 탄창 내 탄약은 제외)
	UPROPERTY(ReplicatedUsing=OnRep_Ammo, BlueprintReadOnly)
	int SpareAmmo = 0;

	// 무기가 이전에 사용된 적이 있는지 (발사, 라운드 경험 등)
	// true인 경우 판매 불가, 땅에 드롭함
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon")
	bool bWasUsed = false;

	// Sets default values for this component's properties
	ABaseWeapon();

	// WeaponID 설정
	UFUNCTION(BlueprintCallable, Category="Weapon", NetMulticast, Reliable)
	void NetMulti_ReloadWeaponData(int32 NewWeaponID);

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

	// 무기 탄약 리셋
	void ServerOnly_ClearAmmo();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual void Fire();
	FVector GetSpreadDirection(const FVector& Direction);
	UFUNCTION(Server, Reliable)
	void ServerRPC_Fire(const FVector& Location, const FVector& Direction);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_PlayFireAnimation();
	UFUNCTION(BlueprintImplementableEvent)
	void PlayFireSound(const USoundBase* Sound, const bool bIsFP, const bool bIsCU, const int Dir, const FName& MuzzleSocketName = FName(NAME_None));
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_PlayReloadAnimation();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_PlayFireSound();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();
	// UFUNCTION(NetMulticast, Reliable)
	// void MulticastRPC_PlayReloadAnim();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopReload();
	
	UFUNCTION()
	void OnRep_Ammo() const;
	virtual void Multicast_SetActive_Implementation(bool bActive) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void EndFire();

	void RecoverRecoilLevel();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Weapon")
	void ServerRPC_StartReload();
	
	UFUNCTION(BlueprintCallable)
	EWeaponCategory GetWeaponCategory() const { return WeaponData->WeaponCategory; }

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> NewDamageEffectClass;

	/*
	 *	PickUp & Drop 관련
	 */
public:
	virtual bool ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const override;
	virtual bool ServerOnly_CanDrop() const override;
	virtual void ServerRPC_PickUp_Implementation(ABaseAgent* Agent) override;
	virtual void ServerRPC_Drop_Implementation() override;
	virtual void ServerRPC_Interact_Implementation(ABaseAgent* InteractAgent) override;
	void ServerOnly_AttachWeapon(ABaseAgent* Agent);

	// 무기 사용 여부 리셋 (라운드 시작 시)
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void ResetUsedStatus();
	void SetWeaponID(const int NewWeaponID);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnMuzzleFlash();
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnTracer(const FVector& Start, const FVector& End);
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnImpactEffect(const FVector& Location, const FRotator& Rotation);
};
