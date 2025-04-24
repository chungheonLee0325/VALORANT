// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPageUI.h"

#include "GameManager/SubsystemSteamManager.h"

void ULobbyPageUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		SubsystemManager->OnFindFirstSteamSessionComplete.AddUObject(this, &ThisClass::JoinMatch);
	}
}

void ULobbyPageUI::FindMatch()
{
	if (true == bIsFindingMatch)
	{
		return;
	}
	
	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		SubsystemManager->FindSessions();
		bIsFindingMatch = true;
	}
}

void ULobbyPageUI::JoinMatch(const FOnlineSessionSearchResult& OnlineSessionSearchResult, bool bArg)
{
	if (false == bIsFindingMatch)
	{
		return;
	}
	bIsFindingMatch = false;
	
	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		if (bArg)
		{
			SubsystemManager->JoinSession(OnlineSessionSearchResult);
		}
	}
}