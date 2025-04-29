// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameMode.h"

#include "MatchGameState.h"
#include "OnlineSessionSettings.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/AgentPlayerController.h"
#include "Player/MatchPlayerController.h"

AMatchGameMode::AMatchGameMode()
{
	// PlayerControllerClass = AMatchPlayerController::StaticClass();
}

void AMatchGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	ValorantGameInstance = Cast<UValorantGameInstance>(GetGameInstance());

	SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>();
	if (SubsystemManager == nullptr)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SubsystemManager is nullptr"), __FUNCTION__);
		return;
	}
	
	const IOnlineSessionPtr SessionInterface = SubsystemManager->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SessionInterface is not valid"), __FUNCTION__);
		return;
	}
	auto* Session = SubsystemManager->GetNamedOnlineSession();
	if (nullptr == Session)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Session is nullptr"), __FUNCTION__);
	}
	else
	{
		Session->SessionSettings.Set(FName("bReadyToTravel"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		SessionInterface->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Try UpdateSession Completed"), __FUNCTION__);
	}

	RequiredPlayerCount = SubsystemManager->ReqMatchAutoStartPlayerCount;
}

void AMatchGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RemainRoundStateTime = FMath::Clamp(RemainRoundStateTime - DeltaSeconds, 0.0f, MaxTime);
	
	AMatchGameState* MatchGameState = GetGameState<AMatchGameState>();
	if (MatchGameState)
	{
		MatchGameState->SetRemainRoundStateTime(RemainRoundStateTime);
	}
}

void AMatchGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
                              FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	NET_LOG(LogTemp, Warning, TEXT("AMainMenuGameMode::PreLogin Options: %s, Address: %s, UniqueId: %s, ErrorMessage: %s"), *Options, *Address, *(UniqueId.IsValid() ? UniqueId->ToString() : FString(TEXT("INVALID"))), *ErrorMessage);
}

void AMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	const auto Address = NewPlayer->GetPlayerNetworkAddress();
	const auto UniqueId = NewPlayer->GetUniqueID();
	NET_LOG(LogTemp, Warning, TEXT("AMainMenuGameMode::PostLogin Address: %s, UniqueId: %d"), *Address, UniqueId);
	auto* Controller = Cast<AMatchPlayerController>(NewPlayer);
	Controller->SetGameMode(this);
}

void AMatchGameMode::OnControllerBeginPlay(AMatchPlayerController* Controller, const FString& Nickname)
{
	FMatchPlayer PlayerInfo;
	PlayerInfo.Controller = Cast<AAgentPlayerController>(Controller);
	PlayerInfo.Nickname = Nickname;
	PlayerInfo.bIsTeamA = MatchPlayers.Num() % 2 == 0;
	MatchPlayers.Add(PlayerInfo);
	++LoggedInPlayerNum;
}

void AMatchGameMode::OnLockIn(AMatchPlayerController* Player, int AgentId)
{
	if (++LockedInPlayerNum >= RequiredPlayerCount)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, All Players Completed Lock In"), __FUNCTION__);
		StartPreRound();
	}
}

bool AMatchGameMode::ReadyToStartMatch_Implementation()
{
	return LoggedInPlayerNum >= RequiredPlayerCount;
}

void AMatchGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	StartSelectAgent();
}

void AMatchGameMode::StartSelectAgent()
{
	SetRoundSubState(ERoundSubState::RSS_SelectAgent);
}

void AMatchGameMode::StartPreRound()
{
	SetRoundSubState(ERoundSubState::RSS_PreRound);
}

void AMatchGameMode::StartBuyPhase()
{
	SetRoundSubState(ERoundSubState::RSS_BuyPhase);
}

void AMatchGameMode::StartInRound()
{
	SetRoundSubState(ERoundSubState::RSS_InRound);
}

void AMatchGameMode::StartEndPhase()
{
	SetRoundSubState(ERoundSubState::RSS_EndPhase);
}

void AMatchGameMode::HandleRoundSubState_SelectAgent()
{
	// 일정 시간 후에 요원 강제 선택 후 라운드 준비
	MaxTime = SelectAgentTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartPreRound, SelectAgentTime);
}

void AMatchGameMode::HandleRoundSubState_PreRound()
{
	// 일정 시간 후에 라운드 시작
	MaxTime = PreRoundTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartInRound, PreRoundTime);
}

void AMatchGameMode::HandleRoundSubState_BuyPhase()
{
	// 일정 시간 후에 라운드 시작
	MaxTime = BuyPhaseTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartInRound, BuyPhaseTime);
}

void AMatchGameMode::HandleRoundSubState_InRound()
{
	// 일정 시간 후에 라운드 종료
	MaxTime = InRoundTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartEndPhase, InRoundTime);
}

void AMatchGameMode::HandleRoundSubState_EndPhase()
{
	// 일정 시간 후에 라운드 재시작
	MaxTime = EndPhaseTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartBuyPhase, EndPhaseTime);
}

void AMatchGameMode::SetRoundSubState(ERoundSubState NewRoundSubState)
{
	if (RoundSubState != NewRoundSubState)
	{
		AMatchGameState* MatchGameState = GetGameState<AMatchGameState>();
		if (nullptr == MatchGameState)
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, MatchGameState is nullptr"), __FUNCTION__);
		}
		RoundSubState = NewRoundSubState;
		MatchGameState->SetRoundSubState(RoundSubState);
		
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
			HandleRoundSubState_EndPhase();
		}

		RemainRoundStateTime = MaxTime;
		MatchGameState->SetRemainRoundStateTime(RemainRoundStateTime);
	}
}
