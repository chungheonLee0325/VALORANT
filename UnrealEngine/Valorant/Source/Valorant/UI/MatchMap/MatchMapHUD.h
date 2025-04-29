// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapHUD.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	UFUNCTION()
	void UpdateTime(float Time);
	
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockTime = nullptr;
};
