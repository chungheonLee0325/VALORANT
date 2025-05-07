// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MatchGameMode.generated.h"

class ABaseAgent;
class AMatchPlayerController;
class AAgentPlayerController;
class APlayerStart;
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

UENUM(BlueprintType)
enum class ERoundEndReason : uint8
{
	ERER_None,
	ERER_Eliminated,
	ERER_Timeout,
	ERER_SpikeActive,
	ERER_SpikeDefuse
};

USTRUCT(BlueprintType)
struct FMatchPlayer
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AAgentPlayerController> Controller = nullptr;
	FString Nickname = "UNKNOWN";
	bool bIsBlueTeam = true;
	int SelectedAgentID = 0;
	bool bIsDead = false;
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
	TArray<FString> RedTeamPlayerNameArray;
	TArray<FString> BlueTeamPlayerNameArray;
	UPROPERTY()
	TObjectPtr<APlayerStart> AgentSelectStartPoint = nullptr;
	UPROPERTY()
	TObjectPtr<APlayerStart> AttackersStartPoint = nullptr;
	UPROPERTY()
	TObjectPtr<APlayerStart> DefendersStartPoint = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSubclassOf<ABaseAgent> AgentClass;
	
public:
	void OnControllerBeginPlay(AMatchPlayerController* Controller, const FString& Nickname);
	void OnLockIn(AMatchPlayerController* Player, int AgentId);
	void OnAgentSelected(AMatchPlayerController* MatchPlayerController, int SelectedAgentID);

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
	bool bReadyToEndMatch = false;
	
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual bool ReadyToEndMatch_Implementation() override;
	virtual void HandleMatchHasEnded() override;
	void StartSelectAgent();
	void StartPreRound();
	void StartBuyPhase();
	void StartInRound();
	void StartEndPhaseByTimeout();
	void StartEndPhaseByEliminated(const bool bBlueWin);
	void StartEndPhaseBySpikeActive();
	void StartEndPhaseBySpikeDefuse();
	void HandleRoundSubState_SelectAgent();
	void HandleRoundSubState_PreRound();
	void HandleRoundSubState_BuyPhase();
	void HandleRoundSubState_InRound();
	void HandleRoundSubState_EndPhase();
	void SetRoundSubState(ERoundSubState NewRoundSubState);

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	void RespawnAll();
	void OnKill(AMatchPlayerController* Killer, AMatchPlayerController* Victim);
	void OnRevive(AMatchPlayerController* Reviver, AMatchPlayerController* Target);
	void OnSpikePlanted(AMatchPlayerController* Planter);
	void OnSpikeDefused(AMatchPlayerController* Defuser);
	
	int TotalRound = 6;
	int CurrentRound = 0;
	int RequiredScore = 4;
	int TeamBlueScore = 0;
	int TeamRedScore = 0;
	int ShiftRound = 4;
	int BlueTeamConsecutiveLosses = 0;
	int RedTeamConsecutiveLosses = 0;
	bool IsShifted() const { return CurrentRound >= ShiftRound; }
	void HandleRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason);

	// 크레딧 시스템 관련 함수
	void AwardRoundEndCredits();
};