// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHUD.h"

#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameManager/MatchGameState.h"
#include "Player/AgentPlayerState.h"

void UMatchMapHUD::NativeConstruct()
{
	Super::NativeConstruct();

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapHUD::UpdateTime);
	GameState->OnTeamScoreChanged.AddDynamic(this, &UMatchMapHUD::UpdateScore);
	GameState->OnRoundSubStateChanged.AddDynamic(this, &UMatchMapHUD::OnRoundSubStateChanged);

	auto* PlayerState = Cast<AAgentPlayerState>(GetOwningPlayer());
	// TODO: 라운드 결과 Delegate Binding
}

void UMatchMapHUD::UpdateTime(float Time)
{
	const int Minute = static_cast<int>(Time / 60);
	const int Seconds = static_cast<int>(Time) % 60;
	const FString TimeStr = FString::Printf(TEXT("%d:%02d"), Minute, Seconds);
	TextBlockTime->SetText(FText::FromString(TimeStr));
}

void UMatchMapHUD::UpdateScore(int TeamBlueScore, int TeamRedScore)
{
	TextBlockBlueScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamBlueScore)));
	TextBlockRedScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamRedScore)));
}

void UMatchMapHUD::OnRoundSubStateChanged(const ERoundSubState RoundSubState)
{
	switch (RoundSubState) {
	case ERoundSubState::RSS_None:
		break;
	case ERoundSubState::RSS_SelectAgent:
		break;
	case ERoundSubState::RSS_PreRound:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, 5.0f);
		break;
	case ERoundSubState::RSS_BuyPhase:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, 5.0f);
		break;
	case ERoundSubState::RSS_InRound:
		break;
	case ERoundSubState::RSS_EndPhase:
		DisplayAnnouncement(EMatchAnnouncement::EMA_Won, 5.0f);
		break;
	}
	DebugRoundSubState(StaticEnum<ERoundSubState>()->GetNameStringByValue(static_cast<int>(RoundSubState)));
}

void UMatchMapHUD::DisplayAnnouncement(EMatchAnnouncement MatchAnnouncement, float DisplayTime)
{
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Visible);
	WidgetSwitcherAnnouncement->SetActiveWidgetIndex(static_cast<int32>(MatchAnnouncement));
	GetWorld()->GetTimerManager().SetTimer(AnnouncementTimerHandle, this, &UMatchMapHUD::HideAnnouncement, DisplayTime, false);
}

void UMatchMapHUD::HideAnnouncement()
{
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Hidden);
}

void UMatchMapHUD::DebugRoundSubState(const FString& RoundSubStateStr)
{
	TextBlockRoundSubStateDbg->SetText(FText::FromString(TEXT("RoundSubState: ") + RoundSubStateStr));
}
