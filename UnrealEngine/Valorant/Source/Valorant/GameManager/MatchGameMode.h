// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MatchGameMode.generated.h"

class AMatchPlayerController;
class USubsystemSteamManager;
class UValorantGameInstance;

USTRUCT(BlueprintType)
struct FMatchPlayer
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AMatchPlayerController> Controller = nullptr;
	FString Nickname = "UNKNOWN";
	bool bIsTeamA = true;
	int SelectedAgentID = 0;
};

/**
 * 
 */
UCLASS()
class VALORANT_API AMatchGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMatchGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual bool ReadyToStartMatch_Implementation() override;

private:
	UPROPERTY()
	TObjectPtr<UValorantGameInstance> ValorantGameInstance = nullptr;
	UPROPERTY()
	TObjectPtr<USubsystemSteamManager> SubsystemManager = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int RequiredPlayerCount = 9999;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int LoggedInPlayerNum = 0;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TArray<FMatchPlayer> MatchPlayers;
	int LockedInPlayerNum = 0;
	
public:
	void OnControllerBeginPlay(AMatchPlayerController* Controller, const FString& Nickname);
	void OnLockIn(AMatchPlayerController* Player, int AgentId);
};