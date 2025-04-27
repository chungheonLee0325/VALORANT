// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerController.h"

#include "Valorant.h"
#include "Blueprint/UserWidget.h"
#include "GameManager/MatchGameMode.h"

AMatchPlayerController::AMatchPlayerController()
{
}

void AMatchPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ServerRPC_NotifyBeginPlay();
}

void AMatchPlayerController::SetGameMode(AMatchGameMode* MatchGameMode)
{
	this->GameMode = MatchGameMode;
}

void AMatchPlayerController::ServerRPC_NotifyBeginPlay_Implementation()
{
	if (nullptr == GameMode)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, GameMode is nullptr"), __FUNCTION__);
		return;
	}

	GameMode->OnControllerBeginPlay(this);
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
		SelectUIWidget->RemoveFromParent();
		SelectUIWidget = nullptr;
	}
}

void AMatchPlayerController::ServerRPC_LockIn_Implementation()
{
	GameMode->OnLockIn(this, 0);
}