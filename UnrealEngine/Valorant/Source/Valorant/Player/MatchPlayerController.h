// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MatchPlayerController.generated.h"

class UMatchMapSelectAgentUI;
class USubsystemSteamManager;
class AMatchGameMode;
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
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;

private:
	// GameMode의 PostLogin 단계에서 주입된다
	UPROPERTY()
	TObjectPtr<AMatchGameMode> GameMode = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMatchMapSelectAgentUI> SelectUIWidgetClass;
	UPROPERTY()
	TObjectPtr<UMatchMapSelectAgentUI> SelectUIWidget = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> HudClass;
	UPROPERTY()
	TObjectPtr<UUserWidget> Hud = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> LoadingUIClass;
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingUI = nullptr;
	
public:
	UFUNCTION(BlueprintCallable)
	UUserWidget* GetMatchMapHud() const { return Hud; }
	void SetGameMode(AMatchGameMode* MatchGameMode);
	UFUNCTION(Server, Reliable)
	void ServerRPC_NotifyBeginPlay(const FString& Name, const FString& RealName);
	UFUNCTION(Client, Reliable)
	void ClientRPC_ShowSelectUI(const TArray<FString>& NewTeamPlayerNameArray);
	UFUNCTION(Client, Reliable)
	void ClientRPC_HideSelectUI();
	UFUNCTION(Client, Reliable)
	void ClientRPC_DisplayHud(bool bDisplay);
	UFUNCTION(Server, Reliable)
	void ServerRPC_LockIn(int SelectedAgentID);
	UFUNCTION(Server, Reliable)
	void ServerRPC_OnAgentSelectButtonClicked(int SelectedAgentID);
	UFUNCTION(Client, Reliable)
	void ClientRPC_OnAgentSelected(const FString& DisplayName, int SelectedAgentID);
	UFUNCTION(Client, Reliable)
	void ClientRPC_OnLockIn(const FString& DisplayName, const int AgentId);
	UFUNCTION(Client, Reliable)
	void ClientRPC_CleanUpSession();
	UFUNCTION(Client, Reliable)
	void ClientRPC_SetViewTargetOnAgentSelectCamera();

	UFUNCTION(Client, Reliable)
	void ClientRPC_SetLoadingUI(bool bDisplay);

	UFUNCTION(Client, Reliable)
	void ClientRPC_PlayTutorialSound();
	UFUNCTION(BlueprintImplementableEvent)
	void PlayTutorialSound();
};