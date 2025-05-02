// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerState.h"

#include "Net/UnrealNetwork.h"

void AMatchPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMatchPlayerState, DisplayName);
	DOREPLIFETIME(AMatchPlayerState, bIsBlueTeam);
	DOREPLIFETIME(AMatchPlayerState, SelectedAgentID);
}

void AMatchPlayerState::OnRep_SelectedAgentID()
{
	// NET_LOG(LogTemp, Warning, TEXT("%hs Called, DisplayName: %s, AgentId: %d"), __FUNCTION__, *DisplayName, SelectedAgentID);
	OnSelectedAgentChanged.Broadcast(DisplayName, SelectedAgentID);
}

void AMatchPlayerState::ServerRPC_NotifyAgentSelected_Implementation(int AgentId)
{
	SelectedAgentID = AgentId;
	OnRep_SelectedAgentID();
}
