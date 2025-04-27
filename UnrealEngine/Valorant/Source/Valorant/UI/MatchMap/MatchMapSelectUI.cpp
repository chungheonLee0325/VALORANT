// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapSelectUI.h"

#include "Valorant.h"
#include "Player/AgentPlayerController.h"

void UMatchMapSelectUI::NativeConstruct()
{
	Super::NativeConstruct();
}

void UMatchMapSelectUI::OnClickedButtonLockIn()
{
	auto* Controller = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (nullptr == Controller)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Controller is nullptr"), __FUNCTION__);
		return;
	}

	Controller->ServerRPC_LockIn();
}