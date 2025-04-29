// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerController.h"

#include "Valorant.h"
#include "Blueprint/UserWidget.h"
#include "GameManager/MatchGameMode.h"
#include "GameManager/SubsystemSteamManager.h"

AMatchPlayerController::AMatchPlayerController()
{
}

void AMatchPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
		const FString Nickname = USubsystemSteamManager::GetDisplayName();
		ServerRPC_NotifyBeginPlay(Nickname);
	}
}

void AMatchPlayerController::SetGameMode(AMatchGameMode* MatchGameMode)
{
	this->GameMode = MatchGameMode;
}

void AMatchPlayerController::ServerRPC_NotifyBeginPlay_Implementation(const FString& Nickname)
{
	if (nullptr == GameMode)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, GameMode is nullptr"), __FUNCTION__);
		return;
	}

	GameMode->OnControllerBeginPlay(this, Nickname);
}

void AMatchPlayerController::ClientRPC_DisplaySelectUI_Implementation(bool bDisplay)
{
	if (bDisplay)
	{
		SelectUIWidget = CreateWidget(this, SelectUIWidgetClass);
		if (nullptr == SelectUIWidget)
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
			return;
		}

		SelectUIWidget->AddToViewport();
	}
	else
	{
		// Pawn 생성하고 세팅하는 동안 로딩 화면 표시
		if (SelectUIWidget)
		{
			SelectUIWidget->RemoveFromParent();
			SelectUIWidget = nullptr;
		}
	}
}

void AMatchPlayerController::ClientRPC_DisplayHud_Implementation(bool bDisplay)
{
	if (bDisplay)
	{
		if (Hud)
		{
			// 이미 존재하면 굳이 또 생성하지 않는다
			return;
		}
		Hud = CreateWidget(this, HudClass);
		if (nullptr == Hud)
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
			return;
		}

		Hud->AddToViewport();
	}
	else
	{
		// Pawn 생성하고 세팅하는 동안 로딩 화면 표시
		if (Hud)
		{
			Hud->RemoveFromParent();
			Hud = nullptr;
		}
	}
}

void AMatchPlayerController::ServerRPC_LockIn_Implementation()
{
	GameMode->OnLockIn(this, 0);
}