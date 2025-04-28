// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapSelectUI.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapSelectUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void OnClickedButtonLockIn();

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRemTime = nullptr;
	UFUNCTION()
	void UpdateTime(float Time);
};
