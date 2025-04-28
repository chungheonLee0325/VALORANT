// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapSelectUI.h"

#include "Valorant.h"
#include "Components/TextBlock.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerController.h"

void UMatchMapSelectUI::NativeConstruct()
{
	Super::NativeConstruct();

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapSelectUI::UpdateTime);
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

void UMatchMapSelectUI::UpdateTime(float Time)
{
	TextBlockRemTime->SetText(FText::FromString(FString::Printf(TEXT("%d"), static_cast<int>(Time))));
}
