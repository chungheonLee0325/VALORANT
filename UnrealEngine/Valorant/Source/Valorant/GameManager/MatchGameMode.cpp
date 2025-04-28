// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameMode.h"

#include "OnlineSessionSettings.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/MatchPlayerController.h"

AMatchGameMode::AMatchGameMode()
{
	// PlayerControllerClass = AMatchPlayerController::StaticClass();
}

void AMatchGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	ValorantGameInstance = Cast<UValorantGameInstance>(GetGameInstance());

	SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>();
	if (SubsystemManager == nullptr)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SubsystemManager is nullptr"), __FUNCTION__);
		return;
	}
	
	const IOnlineSessionPtr SessionInterface = SubsystemManager->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SessionInterface is not valid"), __FUNCTION__);
		return;
	}
	auto* Session = SubsystemManager->GetNamedOnlineSession();
	if (nullptr == Session)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Session is nullptr"), __FUNCTION__);
	}
	else
	{
		Session->SessionSettings.Set(FName("bReadyToTravel"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		SessionInterface->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Try UpdateSession Completed"), __FUNCTION__);
	}

	RequiredPlayerCount = SubsystemManager->ReqMatchAutoStartPlayerCount;
}

void AMatchGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
                              FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	NET_LOG(LogTemp, Warning, TEXT("AMainMenuGameMode::PreLogin Options: %s, Address: %s, UniqueId: %s, ErrorMessage: %s"), *Options, *Address, *(UniqueId.IsValid() ? UniqueId->ToString() : FString(TEXT("INVALID"))), *ErrorMessage);
}

void AMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	const auto Address = NewPlayer->GetPlayerNetworkAddress();
	const auto UniqueId = NewPlayer->GetUniqueID();
	NET_LOG(LogTemp, Warning, TEXT("AMainMenuGameMode::PostLogin Address: %s, UniqueId: %d"), *Address, UniqueId);
	auto* Controller = Cast<AMatchPlayerController>(NewPlayer);
	Controller->SetGameMode(this);
}
bool AMatchGameMode::ReadyToStartMatch_Implementation()
{
	return LoggedInPlayerNum >= RequiredPlayerCount;
}

void AMatchGameMode::OnControllerBeginPlay(AMatchPlayerController* Controller, const FString& Nickname)
{
	FMatchPlayer PlayerInfo;
	PlayerInfo.Controller = Controller;
	PlayerInfo.Nickname = Nickname;
	PlayerInfo.bIsTeamA = MatchPlayers.Num() % 2 == 0;
	MatchPlayers.Add(PlayerInfo);
	++LoggedInPlayerNum;
}

void AMatchGameMode::OnLockIn(AMatchPlayerController* Player, int AgentId)
{
	for (auto& PlayerInfo : MatchPlayers)
	{
		if (PlayerInfo.Controller == Player)
		{
			Player->ClientRPC_DisplaySelectUI(false);
			PlayerInfo.SelectedAgentID = AgentId;
			break;
		}
	}
	
	if (++LockedInPlayerNum >= RequiredPlayerCount)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, All Players Completed Lock In"), __FUNCTION__);
		SetMatchState(MatchState::InProgress);
	}
}