// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMapLobbyPage.h"

#include "MainMapMenuUI.h"
#include "GameManager/SubsystemSteamManager.h"

void UMainMapLobbyPage::NativeConstruct()
{
	Super::NativeConstruct();

	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		SubsystemManager->OnFindFirstSteamSessionComplete.AddUObject(this, &ThisClass::JoinMatch);
	}

	if (MenuUI)
	{
		MenuUI->SetTitle(TEXT("로비"));
	}
}

void UMainMapLobbyPage::Init(UMainMapCoreUI* InitCoreUI)
{
	Super::Init(InitCoreUI);
	MenuUI->Init(CoreUI);
}

void UMainMapLobbyPage::OnClickedButtonStart()
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

void UMainMapLobbyPage::JoinMatch(const FOnlineSessionSearchResult& OnlineSessionSearchResult, bool bArg)
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