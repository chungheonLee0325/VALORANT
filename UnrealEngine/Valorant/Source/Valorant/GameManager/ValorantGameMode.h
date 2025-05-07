// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ValorantGameMode.generated.h"

class UValorantGameInstance;

UCLASS(minimalapi)
class AValorantGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AValorantGameMode();

	UFUNCTION(BlueprintCallable)
	void RespawnAllPlayer();

private:
	UPROPERTY()
	UValorantGameInstance* m_GameInstance;
	
protected:
	virtual void BeginPlay() override;

};



