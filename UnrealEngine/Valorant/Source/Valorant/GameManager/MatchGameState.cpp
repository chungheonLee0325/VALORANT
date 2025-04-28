// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchGameState.h"

#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "GameFramework/PlayerState.h"
#include "Player/MatchPlayerController.h"

void AMatchGameState::BeginPlay()
{
	Super::BeginPlay();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
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
	
	// 캐릭터 선택 UI를 띄우라고 한다
	if (HasAuthority())
	{
		for (const auto PS : PlayerArray)
		{
			if (auto* PC = Cast<AMatchPlayerController>(PS->GetOwner()))
			{
				PC->ClientRPC_DisplaySelectUI(true);
			}
		}
	}
}

void AMatchGameState::HandleMatchHasEnded()
{
	// InProgress 상태일 때 ReadyToEndMatch()가 True가 되면 호출된다 (Tick에서 매번 확인)
	Super::HandleMatchHasEnded();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}

void AMatchGameState::HandleLeavingMap()
{
	Super::HandleLeavingMap();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
}
