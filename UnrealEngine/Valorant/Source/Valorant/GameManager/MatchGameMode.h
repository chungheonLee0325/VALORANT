// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MatchGameMode.generated.h"

class AMatchPlayerController;
class AAgentPlayerController;
class USubsystemSteamManager;
class UValorantGameInstance;

// MatchState가 InProgress일 때 SubState 
UENUM(BlueprintType)
enum class ERoundSubState : uint8
{
	RSS_None,
	RSS_SelectAgent,
	RSS_PreRound,
	RSS_BuyPhase,
	RSS_InRound,
	RSS_EndPhase
};

USTRUCT(BlueprintType)
struct FMatchPlayer
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AAgentPlayerController> Controller = nullptr;
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
	virtual void Tick(float DeltaSeconds) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	UPROPERTY()
	TObjectPtr<UValorantGameInstance> ValorantGameInstance = nullptr;
	UPROPERTY()
	TObjectPtr<USubsystemSteamManager> SubsystemManager = nullptr;
	
	UPROPERTY(Transient)
	ERoundSubState RoundSubState = ERoundSubState::RSS_None;
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

protected:
	FTimerHandle RoundTimerHandle;
	float MaxTime = 0.0f;
	float RemainRoundStateTime = 0.0f;
	float SelectAgentTime = 60.0f;
	float PreRoundTime = 15.0f;		// org: 45.0f
	float BuyPhaseTime = 10.0f;		// org: 30.0f
	float InRoundTime = 20.0f;		// org: 100.0f
	float EndPhaseTime = 10.0f;		// org: 10.0f
	float SpikeActiveTime = 15.0f;	// org: 45.0f
	
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	void StartSelectAgent();
	void StartPreRound();
	void StartBuyPhase();
	void StartInRound();
	void StartEndPhase();
	void HandleRoundSubState_SelectAgent();
	void HandleRoundSubState_PreRound();
	void HandleRoundSubState_BuyPhase();
	void HandleRoundSubState_InRound();
	void HandleRoundSubState_EndPhase();
	void SetRoundSubState(ERoundSubState NewRoundSubState);

	int TotalRound = 6;
	int CurrentRound = 1;
	int RequiredScore = 4;
	int TeamBlueScore = 0;
	int TeamRedScore = 0;
	void IncreaseTeamScore(bool bIsTeamBlue);
};