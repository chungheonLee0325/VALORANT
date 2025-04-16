// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgentBaseWidget.generated.h"

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

public:
};
