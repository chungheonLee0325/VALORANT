// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerController.h"

#include "AgentPlayerState.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Agent/BaseAgent.h"
#include "Blueprint/UserWidget.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Widget/AgentBaseWidget.h"

void AAgentPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	InitCacheGAS();
}

void AAgentPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitCacheGAS();
}

void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// UE_LOG(LogTemp, Warning, TEXT("PC, BeginPlay → %s, LocalRole=%d, IsLocal=%d"),
	// *GetName(), (int32)GetLocalRole(), IsLocalController());
	
	if (IsLocalController())
	{
		m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());

		ABaseAgent* player = Cast<ABaseAgent>(GetPawn());
		if (player == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("게임모드 Default Pawn을 BaseAgent로 설정해주세요."));
			return;
		}
		
		player->BindToDelegatePC(this);
		
		if (AgentWidgetClass == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("PlayerController에 AgentWidget 좀 넣어주세요."));
			return;
		}
		
		// AgentWidget = CreateWidget<UAgentBaseWidget>(this, AgentWidgetClass);
		// if (AgentWidget)
		// {
		// 	AgentWidget->AddToViewport();
		// 	AgentWidget->SetASC(CachedASC);
		// 	AgentWidget->BindToDelegatePC(this);
		// }
	}
}

void AAgentPlayerController::InitCacheGAS()
{
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
	
	CachedABS->OnHealthChanged.AddDynamic(this,&AAgentPlayerController::HandleHealthChanged);
	CachedABS->OnMaxHealthChanged.AddDynamic(this,&AAgentPlayerController::HandleMaxHealthChanged);
	CachedABS->OnArmorChanged.AddDynamic(this,&AAgentPlayerController::HandleArmorChanged);
	CachedABS->OnMoveSpeedChanged.AddDynamic(this,&AAgentPlayerController::HandleMoveSpeedChanged);
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

void AAgentPlayerController::HandleMoveSpeedChanged(float NewSpeed)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, MoveSpeed Changed"));
	OnMoveSpeedChanged_PC.Broadcast(NewSpeed);
}

