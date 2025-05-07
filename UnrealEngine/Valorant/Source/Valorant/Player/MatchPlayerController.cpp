// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerController.h"

#include "Valorant.h"
#include "Blueprint/UserWidget.h"
#include "GameManager/MatchGameMode.h"
#include "GameManager/SubsystemSteamManager.h"
#include "UI/MatchMap/MatchMapSelectAgentUI.h"

AMatchPlayerController::AMatchPlayerController()
{
}

void AMatchPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
		const FString& DisplayName = USubsystemSteamManager::GetDisplayName(GetWorld());
		ServerRPC_NotifyBeginPlay(DisplayName);
	}
}

void AMatchPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, PawnName Is %s"), __FUNCTION__, *InPawn->GetName());
}

void AMatchPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	if (GetPawn() == nullptr)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PawnName Is nullptr"), __FUNCTION__);
	}
	else
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PawnName Is %s"), __FUNCTION__, *GetPawn()->GetName());
	}
}

void AMatchPlayerController::SetGameMode(AMatchGameMode* MatchGameMode)
{
	this->GameMode = MatchGameMode;
}

void AMatchPlayerController::ClientRPC_OnLockIn_Implementation(const FString& DisplayName)
{
	if (nullptr == SelectUIWidget)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
		return;
	}
	SelectUIWidget->OnLockIn(DisplayName);
}

void AMatchPlayerController::ClientRPC_OnAgentSelected_Implementation(const FString& DisplayName, int SelectedAgentID)
{
	if (nullptr == SelectUIWidget)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
		return;
	}
	SelectUIWidget->OnSelectedAgentChanged(DisplayName, SelectedAgentID);
}

void AMatchPlayerController::ServerRPC_NotifyBeginPlay_Implementation(const FString& Name)
{
	if (nullptr == GameMode)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, GameMode is nullptr"), __FUNCTION__);
		return;
	}

	GameMode->OnControllerBeginPlay(this, Name);
}

void AMatchPlayerController::ClientRPC_ShowSelectUI_Implementation(const TArray<FString>& NewTeamPlayerNameArray)
{
	SelectUIWidget = CreateWidget<UMatchMapSelectAgentUI>(this, SelectUIWidgetClass);
	if (nullptr == SelectUIWidget)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
		return;
	}
	SelectUIWidget->OnClickAgentSelectButtonDelegate.AddDynamic(this, &AMatchPlayerController::ServerRPC_OnAgentSelectButtonClicked);
	SelectUIWidget->FillTeamSelectAgentList(NewTeamPlayerNameArray);
	SelectUIWidget->AddToViewport();
}

void AMatchPlayerController::ClientRPC_HideSelectUI_Implementation()
{
	// Pawn 생성하고 세팅하는 동안 로딩 화면 표시
	if (SelectUIWidget)
	{
		SelectUIWidget->RemoveFromParent();
		SelectUIWidget->OnClickAgentSelectButtonDelegate.RemoveAll(this);
		SelectUIWidget = nullptr;
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

void AMatchPlayerController::ServerRPC_OnAgentSelectButtonClicked_Implementation(int SelectedAgentID)
{
	GameMode->OnAgentSelected(this, SelectedAgentID);
}