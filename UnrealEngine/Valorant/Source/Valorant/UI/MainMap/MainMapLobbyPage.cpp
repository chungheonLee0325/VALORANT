// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMapLobbyPage.h"

#include "MainMapMenuUI.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "GameManager/SubsystemSteamManager.h"

void UMainMapLobbyPage::NativeConstruct()
{
	Super::NativeConstruct();

	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		SubsystemManager->OnFindFirstSteamSessionComplete.AddUObject(this, &ThisClass::OnFindFirstSteamSessionComplete);
		SubsystemManager->OnFindSteamSessionsComplete.AddUObject(this, &ThisClass::OnFindSteamSessionComplete);
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
	if (true == bIsFindingSession)
	{
		return;
	}
	
	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		bIsProgressMatchMaking = true;
		bIsFindingSession = true;
		WidgetSwitcher->SetActiveWidgetIndex(1);
		SubsystemManager->FindSessions();
	}
}

void UMainMapLobbyPage::OnClickedButtonCancel()
{
	if (bIsHostingSession)
	{
		if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
		{
			bIsHostingSession = false;
			SubsystemManager->DestroySession();
		}
	}
	bIsProgressMatchMaking = false;
	WidgetSwitcher->SetActiveWidgetIndex(0);
}

void UMainMapLobbyPage::OnFindFirstSteamSessionComplete(const FOnlineSessionSearchResult& OnlineSessionSearchResult, bool bArg)
{
	if (false == bIsFindingSession)
	{
		return;
	}
	bIsFindingSession = false;

	UE_LOG(LogTemp, Warning, TEXT("bArg: %hs"), bArg?"True":"False");
	
	if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		if (bArg)
		{
			SubsystemManager->JoinSession(OnlineSessionSearchResult);
		}
	}
}

void UMainMapLobbyPage::OnFindSteamSessionComplete(const TArray<FOnlineSessionSearchResult>& OnlineSessionSearchResults,
	bool bArg)
{
	if (false == bIsFindingSession)
	{
		return;
	}
	bIsFindingSession = false;

	UE_LOG(LogTemp, Warning, TEXT("%hs Called, Num: %d, bArg: %hs"), __FUNCTION__, OnlineSessionSearchResults.Num(), bArg?"True":"False");

	// 0이 아닌 경우는 관심 없다. OnFindFirstSteamSessionComplete에서 처리를 하기 때문
	// 세션 검색을 성공했지만, 0개인 경우 OnFindFirst... 는 호출되지 않아서 여기서 처리한다.
	if (OnlineSessionSearchResults.Num() == 0)
	{
		if (USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
		{
			if (bArg)
			{
				// 세션이 하나도 없으니 자신이 세션을 직접 만든다.
				bIsHostingSession = true;
				SubsystemManager->CreateSession();
			}
		}
	}
}