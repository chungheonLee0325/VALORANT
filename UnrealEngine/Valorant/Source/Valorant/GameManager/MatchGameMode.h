// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MatchGameMode.generated.h"

class UValorantGameInstance;
/**
 * 
 */
UCLASS()
class VALORANT_API AMatchGameMode : public AGameMode
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<UValorantGameInstance> ValorantGameInstance = nullptr;
	
public:
	AMatchGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
};