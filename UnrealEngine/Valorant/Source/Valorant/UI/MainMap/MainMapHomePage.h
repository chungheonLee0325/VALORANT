// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MainMapWidget.h"
#include "MainMapHomePage.generated.h"

class UButton;
class UMainMapCoreUI;
/**
 * 
 */
UCLASS()
class VALORANT_API UMainMapHomePage : public UMainMapWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void OnClickedButtonPlay();
};
