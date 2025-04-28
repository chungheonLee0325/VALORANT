// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MatchGameMode.generated.h"

class AMatchPlayerController;
class USubsystemSteamManager;
class UValorantGameInstance;

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	Phase_Select,
	Phase_Buy,
	Phase_Round,
	Phase_Intermission,
	Phase_EndMatch
};

// 발로란트 팀 Enum
UENUM(BlueprintType)
enum class EValorantTeam : uint8
{
	Team_None,
	Team_Attacker,
	Team_Defender
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

private:
	UPROPERTY()
	TObjectPtr<UValorantGameInstance> ValorantGameInstance = nullptr;
	UPROPERTY()
	TObjectPtr<USubsystemSteamManager> SubsystemManager = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Multiplayer", meta=(AllowPrivateAccess))
	int RequiredPlayerCount = 9999;
	UPROPERTY(BlueprintReadOnly, Category="Multiplayer", meta=(AllowPrivateAccess))
	int LoggedInPlayerNum = 0;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	EMatchPhase CurrentMatchPhase = EMatchPhase::Phase_Select;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSet<AMatchPlayerController*> PlayerControllerSet;
	
#pragma region Gameflow
	// 게임 진행 관련 변수
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int32 CurrentRound = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int32 MaxRounds = 24;
	
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int32 AttackerScore = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int32 DefenderScore = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	EValorantTeam WinningTeam = EValorantTeam::Team_None;
	
	// 스파이크 관련 변수
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	bool bIsSpikeActive = false;
	
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	bool bIsSpikeDefused = false;

#pragma endregion
	
public:
	void OnControllerBeginPlay(AMatchPlayerController* Controller);
	void StartSelectPhase();
	
	// 게임 진행 관련 함수
	UFUNCTION(BlueprintCallable)
	void StartBuyPhase();
	
	UFUNCTION(BlueprintCallable)
	void StartRound();
	
	UFUNCTION(BlueprintCallable)
	void FinishRound(EValorantTeam WinTeam);
	
	UFUNCTION(BlueprintCallable)
	void OnSpikeActive();
	
	UFUNCTION(BlueprintCallable)
	void OnSpikeDefused();
	
	UFUNCTION(BlueprintCallable)
	void OnPlayerKilled(AMatchPlayerController* Victim, AMatchPlayerController* Killer);
};