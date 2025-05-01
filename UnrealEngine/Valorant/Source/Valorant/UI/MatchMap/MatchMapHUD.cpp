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
	GameState->OnGameStateChanged.AddDynamic(this, &UMatchMapHUD::OnGameStateChanged);
	GameState->OnGameStateChanged.AddDynamic(this, &UMatchMapHUD::DebugGameState);

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

void UMatchMapHUD::OnGameStateChanged(const FString& MatchStateStr, const FString& RoundSubStateStr)
{
	if (RoundSubStateStr == TEXT("RSS_PreRound") || RoundSubStateStr == TEXT("RSS_BuyPhase"))
	{
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, 10.0f);
	}
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

void UMatchMapHUD::DebugGameState(const FString& MatchStateStr, const FString& RoundSubStateStr)
{
	TextBlockMatchStateDbg->SetText(FText::FromString(TEXT("MatchState: ") + MatchStateStr));
	TextBlockRoundSubStateDbg->SetText(FText::FromString(TEXT("RoundSubState: ") + RoundSubStateStr));
}
