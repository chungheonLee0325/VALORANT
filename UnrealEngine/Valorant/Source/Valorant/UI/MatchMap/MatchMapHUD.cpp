// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHUD.h"

#include "Valorant.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerState.h"

void UMatchMapHUD::NativeConstruct()
{
	Super::NativeConstruct();

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapHUD::UpdateTime);
	GameState->OnTeamScoreChanged.AddDynamic(this, &UMatchMapHUD::UpdateScore);
	GameState->OnRoundSubStateChanged.AddDynamic(this, &UMatchMapHUD::OnRoundSubStateChanged);
	GameState->OnRoundEnd.AddDynamic(this, &UMatchMapHUD::OnRoundEnd);
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

void UMatchMapHUD::OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime)
{
	switch (RoundSubState) {
	case ERoundSubState::RSS_None:
		break;
	case ERoundSubState::RSS_SelectAgent:
		break;
	case ERoundSubState::RSS_PreRound:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, TransitionTime);
		break;
	case ERoundSubState::RSS_BuyPhase:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, TransitionTime);
		break;
	case ERoundSubState::RSS_InRound:
		break;
	case ERoundSubState::RSS_EndPhase:
		break;
	}
	DebugRoundSubState(StaticEnum<ERoundSubState>()->GetNameStringByValue(static_cast<int>(RoundSubState)));
}

void UMatchMapHUD::OnRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason, const float TransitionTime)
{
	auto* PlayerState = GetOwningPlayer()->GetPlayerState<AMatchPlayerState>();
	if (PlayerState->bIsBlueTeam)
	{
		DisplayAnnouncement(EMatchAnnouncement::EMA_Won, TransitionTime);
	}
	else
	{
		DisplayAnnouncement(EMatchAnnouncement::EMA_Lost, TransitionTime);
	}
}

void UMatchMapHUD::DisplayAnnouncement(EMatchAnnouncement MatchAnnouncement, float DisplayTime)
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, Idx: %d, TransitionTime: %f"), __FUNCTION__, static_cast<int32>(MatchAnnouncement), DisplayTime);
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
