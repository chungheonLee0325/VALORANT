// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchPlayerController.h"

#include "MatchPlayerState.h"
#include "OnlineSubsystem.h"
#include "Valorant.h"
#include "Blueprint/UserWidget.h"
#include "CharSelect/CharSelectCamera.h"
#include "GameManager/MatchGameMode.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MatchMap/MatchMapSelectAgentUI.h"

AMatchPlayerController::AMatchPlayerController()
{
	ConstructorHelpers::FClassFinder<UUserWidget> loadingUI(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/BluePrint/UI/MatchMap/WBP_MatchMap_LoadingUI.WBP_MatchMap_LoadingUI_C'"));
	if (loadingUI.Succeeded())
	{
		LoadingUIClass = loadingUI.Class;
	}
}

void AMatchPlayerController::BeginPlay()
{
	Super::BeginPlay();

	const FString& DisplayName = USubsystemSteamManager::GetDisplayName(GetWorld());
	const FString& RealName = USubsystemSteamManager::GetDisplayName(GetWorld(), 0, false);
	
	ServerRPC_NotifyBeginPlay(DisplayName, RealName);
}

void AMatchPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (nullptr == InPawn)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Pawn is nullptr"), __FUNCTION__);
		return;
	}
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

void AMatchPlayerController::ClientRPC_SetLoadingUI_Implementation(bool bDisplay)
{
	if (bDisplay)
	{
		if (LoadingUI)
		{
			// 이미 존재하면 굳이 또 생성하지 않는다
			return;
		}
		LoadingUI = CreateWidget(this, LoadingUIClass);
		if (nullptr == LoadingUI)
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
			return;
		}

		LoadingUI->AddToViewport();
	}
	else
	{
		// Pawn 생성하고 세팅하는 동안 로딩 화면 표시
		if (LoadingUI)
		{
			LoadingUI->RemoveFromParent();
			LoadingUI = nullptr;
		}
	}
}

void AMatchPlayerController::ClientRPC_CleanUpSession_Implementation()
{
	if (auto* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		SubsystemManager->DestroySession();
	}
}

void AMatchPlayerController::ClientRPC_OnLockIn_Implementation(const FString& DisplayName, const int AgentId)
{
	if (nullptr == SelectUIWidget)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
		return;
	}
	
	SelectUIWidget->OnLockIn(DisplayName, AgentId);
}

void AMatchPlayerController::ClientRPC_OnAgentSelected_Implementation(const FString& DisplayName, int SelectedAgentID)
{
	if (nullptr == SelectUIWidget)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, SelectUIWidget is nullptr"), __FUNCTION__);
		return;
	}
	
	if (const auto* PS = GetPlayerState<AMatchPlayerState>())
	{
		if (PS->DisplayName == DisplayName)
		{
			const TSubclassOf<AActor> ActorClass = ACharSelectCamera::StaticClass();
			if (auto* CharSelectCamera = Cast<ACharSelectCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), ActorClass)))
			{
				CharSelectCamera->OnSelectedAgent(SelectedAgentID);
			}
		}
	}
	
	SelectUIWidget->OnSelectedAgentChanged(DisplayName, SelectedAgentID);
}

void AMatchPlayerController::ServerRPC_NotifyBeginPlay_Implementation(const FString& Name, const FString& RealName)
{
	if (nullptr == GameMode)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, GameMode is nullptr"), __FUNCTION__);
		return;
	}

	GameMode->OnControllerBeginPlay(this, Name, RealName);
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

void AMatchPlayerController::ServerRPC_LockIn_Implementation(int SelectedAgentID)
{
	GameMode->OnLockIn(this, SelectedAgentID);
}

void AMatchPlayerController::ServerRPC_OnAgentSelectButtonClicked_Implementation(int SelectedAgentID)
{
	GameMode->OnAgentSelected(this, SelectedAgentID);
}

void AMatchPlayerController::ClientRPC_SetViewTargetOnAgentSelectCamera_Implementation()
{
	const TSubclassOf<AActor> ActorClass = ACharSelectCamera::StaticClass();
	if (auto* CharSelectCamera = Cast<ACharSelectCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), ActorClass)))
	{
		NET_LOG(LogTemp, Warning, TEXT("CharSelectCamera Location: %s"), *CharSelectCamera->GetActorLocation().ToString());
		SetViewTarget(CharSelectCamera);
	}
}

void AMatchPlayerController::ClientRPC_PlayTutorialSound_Implementation()
{
	PlayTutorialSound();
}
