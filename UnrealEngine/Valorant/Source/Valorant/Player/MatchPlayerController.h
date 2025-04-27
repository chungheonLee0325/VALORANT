// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameManager/MatchGameMode.h"
#include "MatchPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API AMatchPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMatchPlayerController();
	
protected:
	virtual void BeginPlay() override;

private:
	// GameMode의 PostLogin 단계에서 주입된다
	UPROPERTY()
	TObjectPtr<AMatchGameMode> GameMode = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectUIWidgetClass;
	UPROPERTY()
	TObjectPtr<UUserWidget> SelectUIWidget = nullptr;

public:
	void SetGameMode(AMatchGameMode* MatchGameMode);
	UFUNCTION(Server, Reliable)
	void ServerRPC_NotifyBeginPlay();
	UFUNCTION(Client, Reliable)
	void ClientRPC_DisplaySelectUI(bool bDisplay);
	UFUNCTION(Server, Reliable)
	void ServerRPC_LockIn();
};