// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DatabaseManager.generated.h"

USTRUCT(BlueprintType)
struct FPlayerDTO
{
	GENERATED_BODY()

	UPROPERTY()
	int32 defeat_count = 0;
	UPROPERTY()
	int32 draw_count = 0;
	UPROPERTY()
	int32 elo = 1000;
	UPROPERTY()
	FString platform;
	UPROPERTY()
	FString player_id;
	UPROPERTY()
	int32 total_playseconds = 0;
	UPROPERTY()
	int32 win_count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetPlayerCompleted, const bool, bIsSuccess, const FPlayerDTO&, PlayerDto);

/**
 * 
 */
UCLASS()
class VALORANT_API UDatabaseManager : public UObject
{
	GENERATED_BODY()
	
public:
	static FString DatabaseUrl;
	static UDatabaseManager* GetInstance();
	void GetPlayer(const FString& PlayerId, const FString& Platform, const FOnGetPlayerCompleted& Callback);
	void PostPlayer(const FString& PlayerId, const FString& Platform);

private:
	static UDatabaseManager* Singleton;
};
