// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapShopUI.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapShopUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void OnClickedBuyWeaponButton(const int WeaponId);

	UFUNCTION(BlueprintCallable)
	void OnClickedBuySkillButton(const int SkillId);

	UFUNCTION(BlueprintCallable)
	void OnClickedBuyShiledButton(const int ShieldId);
};
