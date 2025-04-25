// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HttpTestUI.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UHttpTestUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnHttpResponseReceived(int32 ResponseCode, const FString& ResponseBody);
	UFUNCTION(BlueprintCallable)
	void SendRequest();
};