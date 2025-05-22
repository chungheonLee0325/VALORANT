// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ResourceManager/ValorantGameType.h"
#include "AgentAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UAgentAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<ABaseAgent> OwnerAgent = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Move, meta=(AllowPrivateAccess = "true"))
	float Speed = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Move, meta=(AllowPrivateAccess = "true"))
	float Direction = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Look, meta=(AllowPrivateAccess = "true"))
	float Pitch = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Look, meta=(AllowPrivateAccess = "true"))
	float Yaw = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State, meta=(AllowPrivateAccess = "true"))
	bool bIsInAir = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State, meta=(AllowPrivateAccess = "true"))
	bool bIsCrouch = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=State, meta=(AllowPrivateAccess = "true"))
	bool bIsDead = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Interactor, meta=(AllowPrivateAccess = "true"))
	EInteractorType InteractorState = EInteractorType::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Interactor, meta=(AllowPrivateAccess = "true"))
	int InteractorPoseIdx = 0;
	
public:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	void UpdateState();
	UFUNCTION(BlueprintImplementableEvent)
	void OnChangedWeaponState();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquip();
	UFUNCTION(BlueprintImplementableEvent)
	void OnFire();
	UFUNCTION(BlueprintImplementableEvent)
	void OnReload();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpikeActive();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpikeCancel();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpikeDeactive();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpikeDefuseFinish();
};
