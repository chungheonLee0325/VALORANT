// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ValorantGameMode.generated.h"

class UValorantGameInstance;
class AAgentPlayerState;

UCLASS(minimalapi)
class AValorantGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AValorantGameMode();
	
	UFUNCTION(BlueprintCallable)
	void RespawnAllPlayers();
	UFUNCTION(BlueprintCallable)
	void RespawnPlayer(AAgentPlayerState* ps);

	UFUNCTION(BlueprintCallable)
	void ResetAgentAtrributeData(AAgentPlayerState* ps);

	UFUNCTION(BlueprintCallable)
	void SetRespawnLoc(FVector const newLoc) { RespawnLocation = newLoc; }
	UFUNCTION(BlueprintCallable)
	void SetRespawnRot(FRotator const newRot) { RespawnRotation = newRot; }
	
private:
	UPROPERTY()
	UValorantGameInstance* m_GameInstance;

	FVector RespawnLocation = FVector::ZeroVector;
	FRotator RespawnRotation = FRotator::ZeroRotator;
	
protected:
	virtual void BeginPlay() override;

};



