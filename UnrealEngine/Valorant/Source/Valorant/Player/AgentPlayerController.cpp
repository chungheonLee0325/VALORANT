// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerController.h"

#include <GameManager/SubsystemSteamManager.h>

#include "AgentPlayerState.h"
#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Agent/BaseAgent.h"
#include "UI/MatchMap/MatchMapHUD.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Widget/AgentBaseWidget.h"


void AAgentPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	InitGAS();

	if (IsLocalController())
	{
		m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
		// InitAgentWidget();
	}
}

void AAgentPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitGAS();
	
	m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
	// InitAgentWidget();
}

void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// UE_LOG(LogTemp, Warning, TEXT("PC, BeginPlay → %s, LocalRole=%d, IsLocal=%d"),
	// *GetName(), (int32)GetLocalRole(), IsLocalController());
}

void AAgentPlayerController::Client_EnterSpectatorMode_Implementation()
{
	StartSpectatingOnly();
}

void AAgentPlayerController::InitGAS()
{
	//한 게임당 1번 실행되도록
	if (CachedABS && CachedASC)
	{
		// UE_LOG(LogTemp, Warning, TEXT("AgentPC: already Cached"));
		return;	
	}
	
	NET_LOG(LogTemp,Warning,TEXT("이닛 가스"));
	
	if (AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>())
	{
		CachedASC = Cast<UAgentAbilitySystemComponent>(ps->GetAbilitySystemComponent());
		CachedABS = ps->GetBaseAttributeSet();
	}

	if (CachedASC == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("PC, ASC 없음"));
		return;
	}
	if (CachedABS == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("PC, ABS 없음"));
		return;
	}

	CachedABS->ResetAttributeData();
	CachedABS->OnHealthChanged.AddDynamic(this,&AAgentPlayerController::HandleHealthChanged);
	CachedABS->OnMaxHealthChanged.AddDynamic(this,&AAgentPlayerController::HandleMaxHealthChanged);
	CachedABS->OnArmorChanged.AddDynamic(this,&AAgentPlayerController::HandleArmorChanged);
	CachedABS->OnEffectSpeedChanged.AddDynamic(this,&AAgentPlayerController::HandleEffectSpeedChanged);
}

void AAgentPlayerController::InitAgentWidget()
{
	if (AgentWidgetClass == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("PlayerController에 AgentWidget 좀 넣어주세요."));
		return;
	}
	
	if (!AgentWidget)
	{
		AgentWidget = CreateWidget<UMatchMapHUD>(this, AgentWidgetClass);
		AgentWidget->AddToViewport();
		AgentWidget->BindToDelegatePC(CachedASC,this);
	}

	if (AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>())
	{
		AgentWidget->InitUI(ps);
	}
}

void AAgentPlayerController::HandleHealthChanged(float NewHealth)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, Health Changed"));
	OnHealthChanged_PC.Broadcast(NewHealth);
}

void AAgentPlayerController::HandleMaxHealthChanged(float NewMaxHealth)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, MaxHealth Changed"));
	OnMaxHealthChanged_PC.Broadcast(NewMaxHealth);
}

void AAgentPlayerController::HandleArmorChanged(float NewArmor)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, Armor Changed"));
	OnArmorChanged_PC.Broadcast(NewArmor);
}

void AAgentPlayerController::HandleEffectSpeedChanged(float NewSpeed)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, MoveSpeed Changed"));
	OnEffectSpeedChanged_PC.Broadcast(NewSpeed);
}

void AAgentPlayerController::RequestPurchaseAbility(int AbilityID)
{
	if (AbilityID != 0)
	{
		Server_RequestPurchaseAbility(AbilityID);
	}
}

void AAgentPlayerController::Server_RequestPurchaseAbility_Implementation(int AbilityID)
{
	AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		PS->Server_PurchaseAbility(AbilityID);
	}
}

bool AAgentPlayerController::Server_RequestPurchaseAbility_Validate(int AbilityID)
{
	// ToDo 유효성 검사 - 해당 케릭의 스킬 목록이 맞는지...
	return AbilityID != 0;
}

void AAgentPlayerController::RequestOpenShopUI()
{
}
