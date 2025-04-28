// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameMode.h"

#include "OnlineSessionSettings.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/MatchPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Player/Component/CreditComponent.h"

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
	if (LoggedInPlayerNum >= RequiredPlayerCount)
	{
		StartSelectPhase();
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
	PlayerControllerSet.Add(Controller);
}

void AMatchGameMode::OnControllerBeginPlay(AMatchPlayerController* Controller)
{
	if (++LoggedInPlayerNum >= RequiredPlayerCount)
	{
		StartSelectPhase();
	}
}

void AMatchGameMode::StartSelectPhase()
{
	CurrentMatchPhase = EMatchPhase::Phase_Select;
	
	for (auto* Controller : PlayerControllerSet)
	{
		Controller->ClientRPC_DisplaySelectUI();
	}
}

void AMatchGameMode::StartBuyPhase()
{
	CurrentMatchPhase = EMatchPhase::Phase_Buy;
	
	// 모든 플레이어에게 크레딧 리셋 (첫 라운드) 또는 보상
	if (CurrentRound == 0)
	{
		// 첫 라운드 시작: 크레딧 초기화
		for (auto* Controller : PlayerControllerSet)
		{
			AAgentPlayerState* PlayerState = Controller->GetPlayerState<AAgentPlayerState>();
			if (PlayerState && PlayerState->GetCreditComponent())
			{
				PlayerState->GetCreditComponent()->ResetCredits();
			}
		}
	}
	
	// 바이 페이즈 UI 표시
	for (auto* Controller : PlayerControllerSet)
	{
		Controller->ClientRPC_DisplayBuyUI();
	}
}

void AMatchGameMode::StartRound()
{
	CurrentMatchPhase = EMatchPhase::Phase_Round;
	CurrentRound++;
	
	// 바이 페이즈 UI 닫기
	for (auto* Controller : PlayerControllerSet)
	{
		Controller->ClientRPC_HideBuyUI();
	}
}

void AMatchGameMode::FinishRound(EValorantTeam WinTeam)
{
	CurrentMatchPhase = EMatchPhase::Phase_Intermission;
	this->WinningTeam = WinTeam;
	
	// 승리 팀에 점수 추가
	if (WinTeam == EValorantTeam::Team_Attacker)
	{
		AttackerScore++;
	}
	else if (WinTeam == EValorantTeam::Team_Defender)
	{
		DefenderScore++;
	}
	
	// 크레딧 보상 지급
	for (auto* Controller : PlayerControllerSet)
	{
		AAgentPlayerState* PlayerState = Controller->GetPlayerState<AAgentPlayerState>();
		if (PlayerState && PlayerState->GetCreditComponent())
		{
			EValorantTeam PlayerTeam = Controller->GetTeam();
			
			if (PlayerTeam == WinTeam)
			{
				PlayerState->GetCreditComponent()->RewardRoundWin();
			}
			else
			{
				PlayerState->GetCreditComponent()->RewardRoundLose();
			}
		}
	}
	
	// 스파이크 상태 리셋
	bIsSpikeActive = false;
	bIsSpikeDefused = false;
	
	// 게임 종료 체크
	bool bGameEnded = (AttackerScore > MaxRounds / 2) || (DefenderScore > MaxRounds / 2);
	
	if (bGameEnded || CurrentRound >= MaxRounds)
	{
		// 게임 종료
		CurrentMatchPhase = EMatchPhase::Phase_EndMatch;
		
		// 게임 종료 처리
		for (auto* Controller : PlayerControllerSet)
		{
			Controller->ClientRPC_DisplayEndMatchUI();
		}
	}
	else
	{
		// 다음 라운드 준비 - 바이 페이즈로 전환
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AMatchGameMode::StartBuyPhase, 5.0f, false);
	}
}

void AMatchGameMode::OnSpikeActive()
{
	bIsSpikeActive = true;
	
	// 스파이크 설치한 플레이어에게 크레딧 보상
	for (auto* Controller : PlayerControllerSet)
	{
		if (Controller->GetTeam() == EValorantTeam::Team_Attacker && Controller->DidPlantSpike())
		{
			AAgentPlayerState* PlayerState = Controller->GetPlayerState<AAgentPlayerState>();
			if (PlayerState && PlayerState->GetCreditComponent())
			{
				PlayerState->GetCreditComponent()->RewardPlantSpike();
			}
		}
	}
}

void AMatchGameMode::OnSpikeDefused()
{
	bIsSpikeDefused = true;
	bIsSpikeActive = false;
	
	// 스파이크 해제한 플레이어에게 크레딧 보상
	for (auto* Controller : PlayerControllerSet)
	{
		if (Controller->GetTeam() == EValorantTeam::Team_Defender && Controller->DidDefuseSpike())
		{
			AAgentPlayerState* PlayerState = Controller->GetPlayerState<AAgentPlayerState>();
			if (PlayerState && PlayerState->GetCreditComponent())
			{
				PlayerState->GetCreditComponent()->RewardDefuseSpike();
			}
		}
	}
	
	// 수비팀 승리로 라운드 종료
	FinishRound(EValorantTeam::Team_Defender);
}

void AMatchGameMode::OnPlayerKilled(AMatchPlayerController* Victim, AMatchPlayerController* Killer)
{
	// 킬러가 존재하고, 자기 자신을 죽인 것이 아니라면 킬 크레딧 지급
	if (Killer && Killer != Victim)
	{
		AAgentPlayerState* PlayerState = Killer->GetPlayerState<AAgentPlayerState>();
		if (PlayerState && PlayerState->GetCreditComponent())
		{
			PlayerState->GetCreditComponent()->RewardKill();
		}
	}
	
	// 모든 플레이어의 생존 상태 확인
	bool bAllAttackersDead = true;
	bool bAllDefendersDead = true;
	
	for (auto* Controller : PlayerControllerSet)
	{
		if (!Controller->IsDead())
		{
			if (Controller->GetTeam() == EValorantTeam::Team_Attacker)
			{
				bAllAttackersDead = false;
			}
			else if (Controller->GetTeam() == EValorantTeam::Team_Defender)
			{
				bAllDefendersDead = false;
			}
		}
	}
	
	// 라운드 승패 결정
	if (bIsSpikeActive)
	{
		// 스파이크가 설치된 상태
		if (bAllDefendersDead)
		{
			// 수비팀 전멸 - 공격팀 승리
			FinishRound(EValorantTeam::Team_Attacker);
		}
		else if (bAllAttackersDead)
		{
			// 공격팀 전멸 - 결정되지 않음 (스파이크가 터질 때까지 대기)
		}
	}
	else
	{
		// 스파이크가 설치되지 않은 상태
		if (bAllAttackersDead)
		{
			// 공격팀 전멸 - 수비팀 승리
			FinishRound(EValorantTeam::Team_Defender);
		}
		else if (bAllDefendersDead)
		{
			// 수비팀 전멸 - 공격팀 승리
			FinishRound(EValorantTeam::Team_Attacker);
		}
	}
}
