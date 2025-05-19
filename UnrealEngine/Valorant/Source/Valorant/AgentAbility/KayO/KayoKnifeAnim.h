// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/Animaiton/AgentAnimInstance.h"
#include "KayoKnifeAnim.generated.h"

class AKayoKnife;
enum class EKnifeState : uint8;
/**
 * 
 */
UCLASS()
class VALORANT_API UKayoKnifeAnim : public UAgentAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<AKayoKnife> Owner;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EKnifeState State;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsThirdPerson = false;

	UFUNCTION(BlueprintImplementableEvent)
	void OnKnifeEquip();

protected:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};