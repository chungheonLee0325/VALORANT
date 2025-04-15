// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerController.h"

#include "Valorant/GameManager/ValorantGameInstance.h"

void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
	}
}
