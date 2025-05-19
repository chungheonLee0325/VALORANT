// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "KayoKnife.generated.h"

class UKayoKnifeAnim;

UENUM(BlueprintType)
enum class EKnifeState : uint8
{
	EKS_Idle,
	EKS_Throw,
	EKS_Active
};

UCLASS()
class VALORANT_API AKayoKnife : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AKayoKnife();

private:
	const bool bAutoActivate = false;
	const float Speed = 4125;
	const float Gravity = 0.7f;
	const bool bShouldBounce = true;
	// const float Bounciness = 0.2f;
	// const float Friction = 0.8f;
	const float EquipTime = 0.8f;
	const float UnequipTime = 0.5f;
	// const float MaximumAirTime = 2.0f;
	const float SuppressionDuration = 8.0f;
	const float ActiveTime = 1.0f;
	FTimerHandle ActiveTimerHandle;

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	EKnifeState State = EKnifeState::EKS_Idle;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UKayoKnifeAnim> AnimInstance = nullptr;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool bIsThirdPerson = false;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	void ActiveSuppressionZone();

public:
	void OnEquip() const;
	void OnThrow();
	UFUNCTION(BlueprintCallable)
	void SetIsThirdPerson(const bool bNew);
};
