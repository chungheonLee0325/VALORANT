// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchMapHudTopWidget.h"

#include "Components/TextBlock.h"
#include "GameManager/MatchGameState.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerState.h"

void UMatchMapHudTopWidget::BindMatchState(AMatchGameState* Match)
{
    if (BoundMatchState.Get() == Match && !BoundMatchState.IsStale())
    {
        return;
    }
    if (auto* previous = BoundMatchState.Get())
    {
        previous->OnRemainRoundStateTimeChanged.RemoveDynamic(this, &UMatchMapHudTopWidget::UpdateTime);
        previous->OnTeamScoreChanged.RemoveDynamic(this, &UMatchMapHudTopWidget::UpdateScore);
        previous->OnRoundSubStateChanged.RemoveDynamic(this, &UMatchMapHudTopWidget::OnRoundSubStateChanged);
    }

    BoundMatchState = Match;
    LastPhase.Reset();
    LastDisplayedSecond = INDEX_NONE;
    if (Match)
    {
        Match->OnRemainRoundStateTimeChanged.AddUniqueDynamic(this, &UMatchMapHudTopWidget::UpdateTime);
        Match->OnTeamScoreChanged.AddUniqueDynamic(this, &UMatchMapHudTopWidget::UpdateScore);
        Match->OnRoundSubStateChanged.AddUniqueDynamic(this, &UMatchMapHudTopWidget::OnRoundSubStateChanged);
        UpdateTime(Match->GetRemainRoundStateTime());
        UpdateScore(Match->GetTeamBlueScore(), Match->GetTeamRedScore());
        OnRoundSubStateChanged(Match->GetRoundSubState(), 0.0f);
    }
}

void UMatchMapHudTopWidget::NativeDestruct()
{
    BindMatchState(nullptr);
    Super::NativeDestruct();
}

void UMatchMapHudTopWidget::UpdateTime(float Time)
{
    const int32 second = FMath::TruncToInt(FMath::Max(Time, 0.0f));
    if (second != LastDisplayedSecond)
    {
        LastDisplayedSecond = second;
        TextBlockTime->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), second / 60, second % 60)));
    }
}

void UMatchMapHudTopWidget::UpdateScore(int TeamBlueScore, int TeamRedScore)
{
    TextBlockBlueScore->SetText(FText::AsNumber(TeamBlueScore));
    TextBlockRedScore->SetText(FText::AsNumber(TeamRedScore));
}

void UMatchMapHudTopWidget::OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime)
{
    if (LastPhase.IsSet() && LastPhase.GetValue() == RoundSubState)
    {
        return;
    }
    LastPhase = RoundSubState;
    const bool bBuying = RoundSubState == ERoundSubState::RSS_PreRound || RoundSubState == ERoundSubState::RSS_BuyPhase;
    TextBlockTime->SetColorAndOpacity(bBuying ? BuyClockColor : FLinearColor::White);
    if (bBuying)
    {
        InitPlayerCard();
    }
}

void UMatchMapHudTopWidget::OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& Info)
{
    if (const auto* victim = VictimAgent ? VictimAgent->GetPlayerState<AAgentPlayerState>() : nullptr)
    {
        MarkKillOnPlayerCard(victim);
    }
}
