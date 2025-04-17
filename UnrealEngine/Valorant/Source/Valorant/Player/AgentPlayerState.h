// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "AgentPlayerState.generated.h"

class UValorantGameInstance;
class UAgentAbilitySystemComponent;
class UBaseAttributeSet;
/**
 * 
 */
UCLASS()
class VALORANT_API AAgentPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAgentPlayerState();

	UFUNCTION(BlueprintCallable)
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UBaseAttributeSet* GetBaseAttributeSet() const;

	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetHealth() const;
	
	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetMaxHealth() const;
	
	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetArmor() const;
	
	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetMoveSpeed() const;
		
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UAgentAbilitySystemComponent* ASC;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UBaseAttributeSet* BaseAttributeSet;

};
