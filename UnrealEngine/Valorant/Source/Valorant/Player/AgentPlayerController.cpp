// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Widget/AgentBaseWidget.h"

void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());

		if (AgentWidgetClass == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("PlayerController에 AgentWidget 좀 넣어주세요."));
			return;
		}
		
		AgentWidget = CreateWidget<UAgentBaseWidget>(this, AgentWidgetClass);
		if (AgentWidget)
		{
			AgentWidget->AddToViewport();
		}
	}
}
