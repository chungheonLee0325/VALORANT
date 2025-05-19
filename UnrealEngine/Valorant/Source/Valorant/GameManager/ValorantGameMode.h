// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ValorantGameMode.generated.h"

class ABaseWeapon;
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
	void GiveMeleeKnife();

	UFUNCTION(BlueprintCallable)
	void ResetAgentAtrributeData(AAgentPlayerState* ps);

	UFUNCTION(BlueprintCallable)
	void SetRespawnLoc(FVector const newLoc) { RespawnLocation = newLoc; }
	UFUNCTION(BlueprintCallable)
	void SetRespawnRot(FRotator const newRot) { RespawnRotation = newRot; }

	UPROPERTY(EditDefaultsOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSubclassOf<ABaseWeapon> MeleeAsset;
	UPROPERTY(EditDefaultsOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSubclassOf<ABaseWeapon> ClassicAsset;
	
private:
	UPROPERTY()
	UValorantGameInstance* m_GameInstance;

	FVector RespawnLocation = FVector::ZeroVector;
	FRotator RespawnRotation = FRotator::ZeroRotator;
	
protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

};



