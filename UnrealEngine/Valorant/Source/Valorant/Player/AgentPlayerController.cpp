// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerController.h"

#include "AgentPlayerState.h"
#include "Agent/BaseAgent.h"
#include "Blueprint/UserWidget.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Widget/AgentBaseWidget.h"
void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());

		ABaseAgent* player = Cast<ABaseAgent>(GetPawn());
		if (player == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("게임모드 Default Pawn을 BaseAgent로 설정해주세요."));
			return;
		}
		
		if (AgentWidgetClass == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("PlayerController에 AgentWidget 좀 넣어주세요."));
			return;
		}

		UE_LOG(LogTemp, Warning,TEXT("pc비긴플레이에서 위젯생성"));
		AgentWidget = CreateWidget<UAgentBaseWidget>(this, AgentWidgetClass);
		if (AgentWidget)
		{
			AgentWidget->AddToViewport();
		}
	}
}

void AAgentPlayerController::InitUI(const int32 health, const int32 armor, const int32 speed)
{
	ABaseAgent* player = Cast<ABaseAgent>(GetPawn());
	AgentWidget->SetASC(player->GetASC());
	AgentWidget->InitDisplayAgentData(health, armor, speed);
}


