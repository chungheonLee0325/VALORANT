// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpManager.h"
#include "DatabaseManager.generated.h"

USTRUCT(BlueprintType)
struct FUserStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerId;

	UPROPERTY(BlueprintReadWrite)
	FString Platform;

	UPROPERTY(BlueprintReadWrite)
	int32 WinCount = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 DrawCount = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 DefeatCount = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 TotalPlaySeconds = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Elo = 1000;
};

/**
 * 
 */
UCLASS()
class VALORANT_API UDatabaseManager : public UHttpManager
{
	GENERATED_BODY()

public:
	void FetchOrCreateUser();
	virtual void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) override;

private:
	static UDatabaseManager* Singleton;
	FString PlayerId;
	FString Platform;
};