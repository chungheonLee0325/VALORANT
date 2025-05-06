// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Valorant/AbilitySystem/ValorantGameplayTags.h"
#include "AgentBaseWidget.generated.h"

class AAgentPlayerState;
class AAgentPlayerController;
class UAgentAbilitySystemComponent;
struct FGameplayTag;
class UImage;
class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UAgentBaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_AgentName;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_Armor;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_HP;
	
	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_Speed;
	
	UPROPERTY(meta=(BindWidget))
	UImage* img_C;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_C;
	
	UPROPERTY(meta=(BindWidget))
	UImage* img_Q;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_Q;
	
	UPROPERTY(meta=(BindWidget))
	UImage* img_E;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_E;
	
	UPROPERTY(meta=(BindWidget))
	UImage* img_X;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_X;

private:
	FGameplayTag TagQ = FValorantGameplayTags::Get().InputTag_Ability_Q;
	FGameplayTag TagE = FValorantGameplayTags::Get().InputTag_Ability_E;
	FGameplayTag TagC = FValorantGameplayTags::Get().InputTag_Ability_C;
	FGameplayTag TagX = FValorantGameplayTags::Get().InputTag_Ability_X;

	UPROPERTY()
	UAgentAbilitySystemComponent* ASC;
	
public:
	UFUNCTION(BlueprintCallable)
	void SetASC(UAgentAbilitySystemComponent* _asc) { ASC = _asc; }

	UFUNCTION(BlueprintCallable)
	void BindToDelegatePC(UAgentAbilitySystemComponent* _asc, AAgentPlayerController* pc);

	UFUNCTION(BlueprintCallable)
	void InitUI(AAgentPlayerState* ps);

	UFUNCTION(BlueprintCallable)
	void UpdateDisplayHealth(const float health);
	UFUNCTION(BlueprintCallable)
	void UpdateDisplayArmor(const float armor);
	UFUNCTION(BlueprintCallable)
	void UpdateDisplaySpeed(const float speed);
};
