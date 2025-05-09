// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "MatchPlayerState.h"
#include "AgentPlayerState.generated.h"

class UValorantGameInstance;
class UAgentAbilitySystemComponent;
class UBaseAttributeSet;
class UCreditComponent;

// 크레딧 변경 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreditChangedDelegate, int32, NewCredit);

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

	UFUNCTION(BlueprintCallable)
	void SetAgentID(int32 NewAgentID) { m_AgentID = NewAgentID; }
	int32 GetAgentID() const { return m_AgentID; }

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

	// 크레딧 시스템 관련 함수
	UFUNCTION(BlueprintCallable, Category = "Agent|Credits")
	UCreditComponent* GetCreditComponent() const { return CreditComponent; }
	
	// 현재 크레딧 가져오기
	UFUNCTION(BlueprintCallable, Category = "Agent|Credits")
	int32 GetCurrentCredit() const;

	// 크레딧 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Agent|Credits")
	FOnCreditChangedDelegate OnCreditChangedDelegate;
	
	// 크레딧 변경 이벤트 핸들러
	UFUNCTION()
	void OnCreditChanged(int32 NewCredit);
	
	// 클라이언트에 크레딧 정보 동기화 요청
	UFUNCTION(Client, Reliable)
	void Client_SyncCredit(int32 ServerCredit);
	
	// 서버에서 크레딧 정보 동기화 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestCreditSync();

	// 능력 스택 관련 함수
	int32 GetAbilityStack(int32 AbilityID) const;
	int32 ReduceAbilityStack(int32 AbilityID);
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) 
	UAgentAbilitySystemComponent* ASC;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UBaseAttributeSet* BaseAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCreditComponent* CreditComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UValorantGameInstance* m_GameInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 m_AgentID = 0;
	
private:
	// AbilityID, Stack
	UPROPERTY(EditDefaultsOnly)
	TMap<int32, int32> AbilityStacks; 
};
