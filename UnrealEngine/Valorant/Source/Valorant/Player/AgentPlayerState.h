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

// 어빌리티 스택 변경 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityStackChangedDelegate, int32, AbilityID, int32, NewStack);

/**
 * 
 */
UCLASS()
class VALORANT_API AAgentPlayerState : public AMatchPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAgentPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UBaseAttributeSet* GetBaseAttributeSet() const;

	UFUNCTION(BlueprintCallable)
	void SetAgentID(int32 NewAgentID);
	
	UFUNCTION(BlueprintCallable)
	int32 GetAgentID() const;

	// 동기화 타이밍 안맞을경우 메뉴얼 동기화 -> 현재 MatchMapHUD 동기화 타이밍 어긋나서 추가
	UFUNCTION(Reliable, NetMulticast)
	void SyncsAgentID(int AgentID);

	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetHealth() const;
	
	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetMaxHealth() const;
	
	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetArmor() const;
	
	UFUNCTION(BlueprintCallable, Category = "Agent|BaseAttributes")
	float GetEffectSpeed() const;

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

	// 어빌리티 스택 관련 함수들
	UFUNCTION(BlueprintCallable, Category = "Agent|Ability")
	int32 GetAbilityStack(int32 AbilityID) const;

	UFUNCTION(BlueprintCallable, Category = "Agent|Ability")
	int32 ReduceAbilityStack(int32 AbilityID);

	UFUNCTION(BlueprintCallable, Category = "Agent|Ability")
	int32 AddAbilityStack(int32 AbilityID, int32 StacksToAdd = 1);

	UFUNCTION(BlueprintCallable, Category = "Agent|Ability")
	int32 SetAbilityStack(int32 AbilityID, int32 NewStack);

	UFUNCTION(BlueprintCallable, Category = "Agent|Ability")
	bool HasAbilityStacks(int32 AbilityID) const;

	// 최대 스택 제한 가져오기
	UFUNCTION(BlueprintCallable, Category = "Agent|Ability")
	int32 GetMaxAbilityStack(int32 AbilityID) const;

	// 어빌리티 스택 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Agent|Ability")
	FOnAbilityStackChangedDelegate OnAbilityStackChanged;

	// 서버에서 클라이언트로 스택 변경 동기화
	UFUNCTION(Client, Reliable)
	void Client_SyncAbilityStack(int32 AbilityID, int32 NewStack);

	// 서버에 스택 정보 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestAbilityStackSync();

	// 서버에서 모든 스택 정보 전송
	UFUNCTION(Client, Reliable)
	void Client_SyncAllAbilityStacks(const TArray<int32>& AbilityIDs, const TArray<int32>& Stacks);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int32 m_AgentID = 0;
	
private:
	// AbilityID, Stack
	UPROPERTY(EditDefaultsOnly)
	TMap<int32, int32> AbilityStacks; 

	// 스택 변경 시 클라이언트에 동기화
	void SyncAbilityStackToClients(int32 AbilityID, int32 NewStack);
};
