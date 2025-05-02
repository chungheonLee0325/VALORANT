// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "MatchPlayerState.h"
#include "AgentPlayerState.generated.h"

class UValorantGameInstance;
class UAgentAbilitySystemComponent;
class UBaseAttributeSet;
/**
 * 
 */
UCLASS()
class VALORANT_API AAgentPlayerState : public AMatchPlayerState, public IAbilitySystemInterface
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
	float GetEffectSpeed() const;

	// 서버에서 스킬 구매 로직 (PlayerController로부터 RPC 호출됨)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PurchaseAbility(int32 AbilityID);

	int32 GetAbilityStack(int32 AbilityID) const;
	int32 ReduceAbilityStack(int32 AbilityID);
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UAgentAbilitySystemComponent* ASC;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UBaseAttributeSet* BaseAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UValorantGameInstance* m_GameInstance = nullptr;

private:
	UPROPERTY(EditDefaultsOnly)
	int32 CurrentCredit = 0;

	UPROPERTY(EditDefaultsOnly)
	int32 MaxCredit = 100;

	// AbilityID, Stack
	UPROPERTY(EditDefaultsOnly)
	TMap<int32, int32> AbilityStacks; 
};
