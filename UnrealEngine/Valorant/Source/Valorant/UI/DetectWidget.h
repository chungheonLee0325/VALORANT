// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetectWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UDetectWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockName = nullptr;
	
public:
	void SetName(const FString& Name) const;
};
