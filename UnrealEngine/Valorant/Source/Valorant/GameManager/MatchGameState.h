// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MatchGameState.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API AMatchGameState : public AGameState
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void HandleLeavingMap() override;
};
