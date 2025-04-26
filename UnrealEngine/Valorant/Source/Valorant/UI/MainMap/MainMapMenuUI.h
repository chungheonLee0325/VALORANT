// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MainMapWidget.h"
#include "MainMapMenuUI.generated.h"

class UButton;
class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UMainMapMenuUI : public UMainMapWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> TextTitle = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> ButtonPlay = nullptr;

	virtual void NativeConstruct() override;

	void SetTitle(const FString& Title);
	
	UFUNCTION(BlueprintCallable)
	void OnClickedButtonBack();

	UFUNCTION(BlueprintCallable)
	void OnClickedButtonPlay();
};
