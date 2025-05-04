// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerState.h"

#include "Net/UnrealNetwork.h"

void AMatchPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMatchPlayerState, DisplayName);
	DOREPLIFETIME(AMatchPlayerState, bIsBlueTeam);
}
