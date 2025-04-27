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
	
public:
	void OnControllerBeginPlay(AMatchPlayerController* Controller);
	void StartSelectPhase();
};