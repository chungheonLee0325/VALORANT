// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameMode.h"

#include "MatchGameState.h"
#include "OnlineSessionSettings.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/AgentPlayerController.h"
#include "Player/MatchPlayerController.h"
#include "Player/MatchPlayerState.h"

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
	PlayerInfo.bIsBlueTeam = MatchPlayers.Num() % 2 == 0;
	if (PlayerInfo.bIsBlueTeam)
	{
		BlueTeamPlayerNameArray.Add(Nickname);
	}
	else
	{
		RedTeamPlayerNameArray.Add(Nickname);
	}

	if (auto* PlayerState = Controller->GetPlayerState<AMatchPlayerState>())
	{
		PlayerState->bIsBlueTeam = PlayerInfo.bIsBlueTeam;
		PlayerState->DisplayName = Nickname;
	}
	MatchPlayers.Add(PlayerInfo);
	++LoggedInPlayerNum;
}

void AMatchGameMode::OnLockIn(AMatchPlayerController* Player, int AgentId)
{
	auto* PS = Player->GetPlayerState<AMatchPlayerState>();
	if (nullptr == PS)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PS is nullptr"), __FUNCTION__);
		return;
	}
	bool bIsBlueTeam = PS->bIsBlueTeam;
	for (const auto& PlayerInfo : MatchPlayers)
	{
		if (bIsBlueTeam == PlayerInfo.bIsBlueTeam)
		{
			PlayerInfo.Controller->ClientRPC_OnLockIn(PS->DisplayName);
		}
	}
	
	if (++LockedInPlayerNum >= RequiredPlayerCount)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, All Players Completed Lock In"), __FUNCTION__);
		StartPreRound();
	}
}

void AMatchGameMode::OnAgentSelected(AMatchPlayerController* MatchPlayerController, int SelectedAgentID)
{
	auto* PS = MatchPlayerController->GetPlayerState<AMatchPlayerState>();
	if (nullptr == PS)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PS is nullptr"), __FUNCTION__);
		return;
	}
	bool bIsBlueTeam = PS->bIsBlueTeam;
	for (const auto& PlayerInfo : MatchPlayers)
	{
		if (bIsBlueTeam == PlayerInfo.bIsBlueTeam)
		{
			PlayerInfo.Controller->ClientRPC_OnAgentSelected(PS->DisplayName, SelectedAgentID);
		}
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

bool AMatchGameMode::ReadyToEndMatch_Implementation()
{
	return bReadyToEndMatch;
}

void AMatchGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	// TODO: 결과 레벨로 전환?
}

void AMatchGameMode::StartSelectAgent()
{
	SetRoundSubState(ERoundSubState::RSS_SelectAgent);
}

void AMatchGameMode::StartPreRound()
{
	++CurrentRound;
	SetRoundSubState(ERoundSubState::RSS_PreRound);
}

void AMatchGameMode::StartBuyPhase()
{
	++CurrentRound;
	SetRoundSubState(ERoundSubState::RSS_BuyPhase);
}

void AMatchGameMode::StartInRound()
{
	SetRoundSubState(ERoundSubState::RSS_InRound);
}

void AMatchGameMode::StartEndPhaseByTimeout()
{
	bool bBlueWin = IsShifted(); // Blue가 선공이니까 false라면 공격->진다, true라면 수비->이긴다
	SetRoundSubState(ERoundSubState::RSS_EndPhase);
	HandleRoundEnd(bBlueWin, ERoundEndReason::ERER_Timeout);
}

void AMatchGameMode::StartEndPhaseByEliminated(const bool bBlueWin)
{
	SetRoundSubState(ERoundSubState::RSS_EndPhase);
	HandleRoundEnd(bBlueWin, ERoundEndReason::ERER_Eliminated);
}

void AMatchGameMode::StartEndPhaseBySpikeActive()
{
	bool bBlueWin = !IsShifted(); // Blue가 선공이니까 false라면 공격->이긴다, true라면 수비->진다
	SetRoundSubState(ERoundSubState::RSS_EndPhase);
	HandleRoundEnd(bBlueWin, ERoundEndReason::ERER_SpikeActive);
}

void AMatchGameMode::StartEndPhaseBySpikeDefuse()
{
	bool bBlueWin = IsShifted(); // Blue가 선공이니까 false라면 공격->해체당했으니 진다, true라면 수비->해체했으니 이긴다
	SetRoundSubState(ERoundSubState::RSS_EndPhase);
	HandleRoundEnd(bBlueWin, ERoundEndReason::ERER_SpikeDefuse);
}

void AMatchGameMode::HandleRoundSubState_SelectAgent()
{
	for (const FMatchPlayer& MatchPlayer : MatchPlayers)
	{
		if (MatchPlayer.bIsBlueTeam)
		{
			MatchPlayer.Controller->ClientRPC_ShowSelectUI(BlueTeamPlayerNameArray);
		}
		else
		{
			MatchPlayer.Controller->ClientRPC_ShowSelectUI(RedTeamPlayerNameArray);
		}
	}
	// 일정 시간 후에 요원 강제 선택 후 라운드 준비
	MaxTime = SelectAgentTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartPreRound, SelectAgentTime);
}

void AMatchGameMode::HandleRoundSubState_PreRound()
{
	if (CurrentRound == ShiftRound)
	{
		// TODO: 공수교대
	}
	RespawnAll();
	// 일정 시간 후에 라운드 시작
	MaxTime = PreRoundTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartInRound, PreRoundTime);
}

void AMatchGameMode::HandleRoundSubState_BuyPhase()
{
	RespawnAll();
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
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartEndPhaseByTimeout, InRoundTime);
}

void AMatchGameMode::HandleRoundSubState_EndPhase()
{
	// TODO: 라운드 상황에 따라 BuyPhase로 전환할 것인지 InRound로 전환할 것인지 아예 매치가 끝난 상태로 전환할 것인지 판단
	// 공수교대(->InRound) 조건: 3라운드가 끝나고 4라운드 시작되는 시점
	// 매치 종료 조건: 4승을 먼저 달성한 팀이 있는 경우 (6전 4선승제, 만약 3:3일 경우 단판 승부전)
	
	// 일정 시간 후에 라운드 재시작
	MaxTime = EndPhaseTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	if (CurrentRound == TotalRound)
	{
		// TODO: 
	}
	else if (CurrentRound == TotalRound - 1)
	{
		GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartPreRound, EndPhaseTime);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartBuyPhase, EndPhaseTime);
	}
	
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
		MatchGameState->SetRoundSubState(RoundSubState, MaxTime);
		
		RemainRoundStateTime = MaxTime;
		MatchGameState->SetRemainRoundStateTime(RemainRoundStateTime);
	}
}

void AMatchGameMode::RespawnAll()
{
	for (auto& PlayerInfo : MatchPlayers)
	{
		if (PlayerInfo.bIsDead)
		{
			PlayerInfo.bIsDead = false;
			// TODO: 체력 등 정상화
			// TODO: 팀 & 공수교대 여부에 따라 처리
		}
	}
}

void AMatchGameMode::OnKill(AMatchPlayerController* Killer, AMatchPlayerController* Victim)
{
	int TeamBlueSurvivorNum = 0;
	int TeamRedSurvivorNum = 0;
	for (auto& PlayerInfo : MatchPlayers)
	{
		if (PlayerInfo.Controller == Cast<AAgentPlayerController>(Victim))
		{
			PlayerInfo.bIsDead = true;
		}
		
		if (PlayerInfo.bIsBlueTeam && false == PlayerInfo.bIsDead)
		{
			++TeamBlueSurvivorNum;
		}
		else
		{
			++TeamRedSurvivorNum;
		}
	}
	
	if (TeamBlueSurvivorNum == 0 && TeamRedSurvivorNum == 0)
	{
		// TODO: 동시에 죽는것을 감지하려면 시간차를 두거나 HandleKill을 배열로 받는 변경 필요
		// TODO: 스파이크 설치 여부에 따라 결정
		StartEndPhaseByEliminated(true);
	}
	else if (TeamBlueSurvivorNum == 0)
	{
		StartEndPhaseByEliminated(false);
	}
	else if (TeamRedSurvivorNum == 0)
	{
		StartEndPhaseByEliminated(true);
	}
}

void AMatchGameMode::OnRevive(AMatchPlayerController* Reviver, AMatchPlayerController* Target)
{
	for (auto& PlayerInfo : MatchPlayers)
	{
		if (PlayerInfo.Controller == Cast<AAgentPlayerController>(Target))
		{
			PlayerInfo.bIsDead = false;
			break;
		}
	}
}

void AMatchGameMode::OnSpikePlanted(AMatchPlayerController* Planter)
{
	MaxTime = SpikeActiveTime;
	RemainRoundStateTime = MaxTime;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RoundTimerHandle, this, &AMatchGameMode::StartEndPhaseBySpikeActive, SpikeActiveTime);
}

void AMatchGameMode::OnSpikeDefused(AMatchPlayerController* Defuser)
{
	StartEndPhaseBySpikeDefuse();
}

void AMatchGameMode::HandleRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason)
{
	// TODO: 규칙에 따라 크레딧 지급

	if (bBlueWin)
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
	MatchGameState->MulticastRPC_HandleRoundEnd(bBlueWin, RoundEndReason);
}
