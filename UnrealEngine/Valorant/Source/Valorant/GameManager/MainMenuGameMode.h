// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MainMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API AMainMenuGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	UFUNCTION(BlueprintImplementableEvent)
	void OnMatchFound();
};
