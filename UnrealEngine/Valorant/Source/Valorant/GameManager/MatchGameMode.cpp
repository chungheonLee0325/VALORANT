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
	IncreaseTeamScore(true);
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
	// TODO: 스파이크가 설치되면 기존 타이머 Clear 및 Set 필요
	
	// TODO: 킬이 발생하면 라운드 종료 요건 만족하는지 판단 후 즉시 EndPhase로 전환 (스파이크 해제도 마찬가지)
	// 공격팀 승리 조건: 수비팀이 먼저 전멸되거나 스파이크 설치 후 폭파 성공
	// 수비팀 승리 조건: 공격팀이 먼저 전멸, 스파이크 해제, 라운드 시간 초과
	// 만약, 동시에 전멸되는 경우 스파이크 설치 유무에 따라 승리 진영이 달라진다 (미설치->수비승리, 설치->공격승리)
	
	// 일정 시간 후에 라운드 종료
	MaxTime = InRoundTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartEndPhase, InRoundTime);
}

void AMatchGameMode::HandleRoundSubState_EndPhase()
{
	// TODO: 라운드 상황에 따라 BuyPhase로 전환할 것인지 InRound로 전환할 것인지 아예 매치가 끝난 상태로 전환할 것인지 판단
	// 공수교대(->InRound) 조건: 3라운드가 끝나고 4라운드 시작되는 시점
	// 매치 종료 조건: 4승을 먼저 달성한 팀이 있는 경우 (6전 4선승제, 만약 3:3일 경우 단판 승부전)
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

void AMatchGameMode::IncreaseTeamScore(bool bIsTeamBlue)
{
	if (bIsTeamBlue)
	{
		++TeamBlueScore;
	}
	else
	{
		++TeamRedScore;
	}
	AMatchGameState* MatchGameState = GetGameState<AMatchGameState>();
	if (nullptr == MatchGameState)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, MatchGameState is nullptr"), __FUNCTION__);
	}
	MatchGameState->SetTeamScore(TeamBlueScore, TeamRedScore);
}
