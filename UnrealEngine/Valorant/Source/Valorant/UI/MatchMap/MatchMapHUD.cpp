// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHUD.h"

#include "OnlineSubsystemUtils.h"
#include "Valorant.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"

void UMatchMapHUD::SetTrueVo()
{
	bPlayed60SecLeftVo = true;
	bPlayed30SecLeftVo = true;
	bPlayed10SecLeftVo = true;
}

void UMatchMapHUD::SetFalseVo()
{
	bPlayed60SecLeftVo = false;
	bPlayed30SecLeftVo = false;
	bPlayed10SecLeftVo = false;
}

void UMatchMapHUD::NativeConstruct()
{
	Super::NativeConstruct();

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapHUD::UpdateTime);
	GameState->OnTeamScoreChanged.AddDynamic(this, &UMatchMapHUD::UpdateScore);
	GameState->OnRoundSubStateChanged.AddDynamic(this, &UMatchMapHUD::OnRoundSubStateChanged);
	GameState->OnRoundEnd.AddDynamic(this, &UMatchMapHUD::OnRoundEnd);

	AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (pc)
	{
		BindToDelegatePC(pc->GetCacehdASC(),pc);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("MatchMap HUD, PC NULL"));
		return;
	}

	if (AAgentPlayerState* ps = pc->GetPlayerState<AAgentPlayerState>())
	{
		InitUI(ps);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("MatchMap HUD, PS NULL"));
	}
}

void UMatchMapHUD::UpdateTime(float Time)
{
	const int Minute = static_cast<int>(Time / 60);
	const int Seconds = static_cast<int>(Time) % 60;
	const FString TimeStr = FString::Printf(TEXT("%d:%02d"), Minute, Seconds);
	TextBlockTime->SetText(FText::FromString(TimeStr));
	if (Time <= 60)
	{
		if (false == bPlayed60SecLeftVo && FMath::IsNearlyEqual(Time, 60.f, 0.5f))
		{
			PlayRemTimeVO(0);
			bPlayed60SecLeftVo = true;
		}
		if (Time <= 30)
		{
			if (false == bPlayed30SecLeftVo && FMath::IsNearlyEqual(Time, 30.f, 0.5f))
			{
				PlayRemTimeVO(1);
				bPlayed30SecLeftVo = true;
			}
			if (Time <= 10)
			{
				if (false == bPlayed10SecLeftVo && FMath::IsNearlyEqual(Time, 10.f, 0.5f))
				{
					PlayRemTimeVO(2);
					bPlayed10SecLeftVo = true;
				}
			}
		}
	}
}

void UMatchMapHUD::UpdateScore(int TeamBlueScore, int TeamRedScore)
{
	TextBlockBlueScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamBlueScore)));
	TextBlockRedScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamRedScore)));
}

void UMatchMapHUD::OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime)
{
	SetTrueVo();
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
		SetFalseVo();
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
		PlayRoundEndVFX(true);
	}
	else
	{
		DisplayAnnouncement(EMatchAnnouncement::EMA_Lost, TransitionTime);
		PlayRoundEndVFX(false);
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

void UMatchMapHUD::UpdateDisplayHealth(const float health)
{
	txt_HP->SetText(FText::AsNumber(health));
}

void UMatchMapHUD::UpdateDisplayArmor(const float armor)
{
	txt_Armor->SetText(FText::AsNumber(armor));
}

void UMatchMapHUD::BindToDelegatePC(UAgentAbilitySystemComponent* asc, AAgentPlayerController* pc)
{
	pc->OnHealthChanged_PC.AddDynamic(this, &UMatchMapHUD::UpdateDisplayHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &UMatchMapHUD::UpdateDisplayArmor);

	if (asc == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AgentWidget, ASC NULL"));
		return;
	}
	ASC = asc;
}

void UMatchMapHUD::InitUI(AAgentPlayerState* ps)
{
	txt_HP->SetText(FText::AsNumber(ps->GetHealth()));
	txt_Armor->SetText(FText::AsNumber(ps->GetArmor()));
}

void UMatchMapHUD::DebugRoundSubState(const FString& RoundSubStateStr)
{
	TextBlockRoundSubStateDbg->SetText(FText::FromString(TEXT("RoundSubState: ") + RoundSubStateStr));
}
