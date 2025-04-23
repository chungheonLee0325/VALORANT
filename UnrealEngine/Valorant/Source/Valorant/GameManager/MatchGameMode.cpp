// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameMode.h"

#include "Valorant.h"
#include "GameManager/ValorantGameInstance.h"

AMatchGameMode::AMatchGameMode()
{
}

void AMatchGameMode::BeginPlay()
{
	Super::BeginPlay();
	NET_LOG(LogTemp, Warning, TEXT("AMatchGameMode::BeginPlay"));
	ValorantGameInstance = Cast<UValorantGameInstance>(GetGameInstance());
	ValorantGameInstance->BroadcastTravel();
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
