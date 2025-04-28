// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapSelectAgentUI.h"

#include "Valorant.h"
#include "Components/TextBlock.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerController.h"

void UMatchMapSelectAgentUI::NativeConstruct()
{
	Super::NativeConstruct();

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapSelectAgentUI::UpdateTime);
}

void UMatchMapSelectAgentUI::OnClickedButtonLockIn()
{
	auto* Controller = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (nullptr == Controller)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Controller is nullptr"), __FUNCTION__);
		return;
	}

	Controller->ServerRPC_LockIn();
}

void UMatchMapSelectAgentUI::UpdateTime(float Time)
{
	TextBlockRemTime->SetText(FText::FromString(FString::Printf(TEXT("%d"), static_cast<int>(Time))));
}
