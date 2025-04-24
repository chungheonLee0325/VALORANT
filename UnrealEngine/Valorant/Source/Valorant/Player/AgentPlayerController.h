// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AgentPlayerController.generated.h"

class UBaseAttributeSet;
class UAgentAbilitySystemComponent;
class UAgentBaseWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged_PC, float, newHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChanged_PC, float, newMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmorChanged_PC, float, newArmor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectSpeedChanged_PC, float, newSpeed);

UCLASS()
class VALORANT_API AAgentPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UAgentBaseWidget* GetAgentWidget() const { return AgentWidget; }
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Widget")
	TSubclassOf<UUserWidget> AgentWidgetClass;

	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged_PC OnHealthChanged_PC;
	UPROPERTY(BlueprintAssignable)
	FOnMaxHealthChanged_PC OnMaxHealthChanged_PC;
	UPROPERTY(BlueprintAssignable)
	FOnArmorChanged_PC OnArmorChanged_PC;
	UPROPERTY(BlueprintAssignable)
	FOnEffectSpeedChanged_PC OnEffectSpeedChanged_PC;

	// 클라이언트에서 호출 -> 서버로 스킬 구매 요청
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestPurchaseAbility(int AbilityID);

	// 서버에서 실행될 실제 구매 요청 함수 (위 함수 내부에서 호출)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPurchaseAbility(int AbilityID);

	// ToDo : 상점 UI 열기 요청
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RequestOpenShopUI(); 
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UValorantGameInstance* m_GameInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAgentAbilitySystemComponent* CachedASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBaseAttributeSet* CachedABS = nullptr ;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAgentBaseWidget* AgentWidget;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void InitCacheGAS();

	UFUNCTION()
	void CreateAgentWidget();
	
	UFUNCTION()
	void HandleHealthChanged(float NewHealth);
	UFUNCTION()
	void HandleMaxHealthChanged(float NewMaxHealth);
	UFUNCTION(BlueprintCallable)
	void HandleArmorChanged(float NewArmor);
	UFUNCTION()
	void HandleEffectSpeedChanged(float NewSpeed);
	
};
