// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerState.h"

#include "GameManager/MatchGameState.h"
#include "Net/UnrealNetwork.h"

void AMatchPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMatchPlayerState, DisplayName);
	DOREPLIFETIME(AMatchPlayerState, bIsBlueTeam);
	DOREPLIFETIME(AMatchPlayerState, bIsAttacker);
}

void AMatchPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (auto* GameState = GetWorld()->GetGameState<AMatchGameState>())
	{
		GameState->OnShift.AddDynamic(this, &AMatchPlayerState::ServerRPC_OnShift);
	}
}

void AMatchPlayerState::ServerRPC_OnShift_Implementation()
{
	bIsAttacker = !bIsAttacker;
}
