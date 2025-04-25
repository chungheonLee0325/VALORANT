// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameMode.h"

#include "OnlineSessionSettings.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "GameManager/ValorantGameInstance.h"

AMatchGameMode::AMatchGameMode()
{
}

void AMatchGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	ValorantGameInstance = Cast<UValorantGameInstance>(GetGameInstance());

	if (const USubsystemSteamManager* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
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
			return;
		}
		Session->SessionSettings.Set(FName("bReadyToTravel"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		SessionInterface->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Try UpdateSession Completed"), __FUNCTION__);
	}
	else
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SubsystemManager is nullptr"), __FUNCTION__);
	}
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
}
