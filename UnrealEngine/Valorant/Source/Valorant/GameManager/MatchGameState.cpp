// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameState.h"

#include "MatchGameMode.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "Net/UnrealNetwork.h"
#include "Player/AgentPlayerController.h"
#include "Player/MatchPlayerController.h"

void AMatchGameState::BeginPlay()
{
	Super::BeginPlay();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMatchGameState, RoundSubState);
	DOREPLIFETIME(AMatchGameState, RemainRoundStateTime);
	DOREPLIFETIME(AMatchGameState, TeamBlueScore);
	DOREPLIFETIME(AMatchGameState, TeamRedScore);
	DOREPLIFETIME(AMatchGameState, TransitionTime);
}

void AMatchGameState::HandleMatchIsWaitingToStart()
{
	// Server측 맵 로딩 완료 (큰 의미 없는 이벤트)
	Super::HandleMatchIsWaitingToStart();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::HandleMatchHasStarted()
{
	// WaitingToStart 상태일 때 ReadyToStartMatch()가 True가 되면 호출된다 (Tick에서 매번 확인)
	// ReadyToStartMatch는 기본적으로 GameMode에 구현되어 있지만 MatchGameMode에서 override하였음.
	// 모든 플레이어가 다 맵 로딩(PlayerController BeginPlay)을 마치고 매치가 본격적으로 시작된 단계
	Super::HandleMatchHasStarted();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::HandleMatchHasEnded()
{
	// InProgress 상태일 때 ReadyToEndMatch()가 True가 되면 호출된다 (Tick에서 매번 확인)
	Super::HandleMatchHasEnded();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	// for (const auto PS : PlayerArray)
	// {
	// 	auto* PC = Cast<AAgentPlayerController>(PS->GetPlayerController());
	// 	PC->OnMatchEnd(TeamBlueScore >= TeamRedScore);
	// }
}

void AMatchGameState::HandleLeavingMap()
{
	Super::HandleLeavingMap();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::OnRep_RoundSubState()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	if (RoundSubState == ERoundSubState::RSS_SelectAgent)
	{
		HandleRoundSubState_SelectAgent();
	}
	else if (RoundSubState == ERoundSubState::RSS_PreRound)
	{
		HandleRoundSubState_PreRound();
	}
	else if (RoundSubState == ERoundSubState::RSS_BuyPhase)
	{
		HandleRoundSubState_BuyPhase();
	}
	else if (RoundSubState == ERoundSubState::RSS_InRound)
	{
		HandleRoundSubState_InRound();
	}
	else if (RoundSubState == ERoundSubState::RSS_EndPhase)
	{
		HandleRoundSubState_EndRound();
	}
	OnRoundSubStateChanged.Broadcast(RoundSubState, TransitionTime);
}

void AMatchGameState::OnRep_RemainRoundStateTime()
{
	OnRemainRoundStateTimeChanged.Broadcast(RemainRoundStateTime);
}

void AMatchGameState::HandleRoundSubState_SelectAgent()
{
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetWorld()->GetFirstPlayerController());
	if (nullptr == PC)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PC is nullptr"), __FUNCTION__);
		return;
	}
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);

	// PC->ClientRPC_ShowSelectUI(true);
}

void AMatchGameState::HandleRoundSubState_PreRound()
{
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetWorld()->GetFirstPlayerController());
	if (nullptr == PC)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PC is nullptr"), __FUNCTION__);
		return;
	}
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	PC->ClientRPC_HideSelectUI();
	PC->ClientRPC_DisplayHud(true);
	PC->ClientRPC_PlayTutorialSound();
}

void AMatchGameState::HandleRoundSubState_BuyPhase()
{
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetWorld()->GetFirstPlayerController());
	if (nullptr == PC)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PC is nullptr"), __FUNCTION__);
		return;
	}
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	PC->ClientRPC_DisplayHud(true);
	PC->ClientRPC_PlayTutorialSound();
}

void AMatchGameState::HandleRoundSubState_InRound()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::HandleRoundSubState_EndRound()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::OnRep_TeamScore()
{
	OnTeamScoreChanged.Broadcast(TeamBlueScore, TeamRedScore);
}

void AMatchGameState::SetRoundSubState(ERoundSubState NewRoundSubState, const float NewTransitionTime)
{
	if (HasAuthority())
	{
		RoundSubState = NewRoundSubState;
		TransitionTime = NewTransitionTime;
		OnRep_RoundSubState();
	}
}

void AMatchGameState::SetRemainRoundStateTime(float NewRemainRoundStateTime)
{
	if (HasAuthority())
	{
		RemainRoundStateTime = NewRemainRoundStateTime;
		OnRep_RemainRoundStateTime();
	}
}

void AMatchGameState::MulticastRPC_HandleRoundEnd_Implementation(bool bBlueWin, ERoundEndReason RoundEndReason)
{
	OnRoundEnd.Broadcast(bBlueWin, RoundEndReason, TransitionTime);
}

void AMatchGameState::SetTeamScore(int NewTeamBlueScore, int NewTeamRedScore)
{
	if (HasAuthority())
	{
		this->TeamBlueScore = NewTeamBlueScore;
		this->TeamRedScore = NewTeamRedScore;
		OnRep_TeamScore();
	}
}

bool AMatchGameState::CanOpenShop() const
{
	return (RoundSubState == ERoundSubState::RSS_BuyPhase || RoundSubState == ERoundSubState::RSS_PreRound);
}

ERoundSubState AMatchGameState::GetRoundSubState() const
{
	return RoundSubState;
}

void AMatchGameState::MulticastRPC_CloseAllShops_Implementation()
{
	// 상점 닫기 이벤트 브로드캐스트
	OnShopClosed.Broadcast();
}

void AMatchGameState::MulticastRPC_OnSpikePlanted_Implementation(AMatchPlayerController* Planter)
{
	// 모든 클라이언트에게 스파이크 설치 이벤트 브로드캐스트
	OnSpikePlanted.Broadcast(Planter);
}

void AMatchGameState::MulticastRPC_OnSpikeDefused_Implementation(AMatchPlayerController* Defuser)
{
	// 모든 클라이언트에게 스파이크 해제 이벤트 브로드캐스트
	OnSpikeDefused.Broadcast(Defuser);
}

void AMatchGameState::MulticastRPC_OnShift_Implementation()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnShift.Broadcast();
}

void AMatchGameState::MulticastRPC_OnMatchEnd_Implementation(const bool bBlueWin)
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnMatchEnd.Broadcast(bBlueWin);
}