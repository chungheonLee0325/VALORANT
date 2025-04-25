// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMapWidget.generated.h"

class UMainMapCoreUI;
/**
 * 
 */
UCLASS()
class VALORANT_API UMainMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMainMapCoreUI> CoreUI = nullptr;

	virtual void Init(UMainMapCoreUI* InitCoreUI) { CoreUI = InitCoreUI; };
};
