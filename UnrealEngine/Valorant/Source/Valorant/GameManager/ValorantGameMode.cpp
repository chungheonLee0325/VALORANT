// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValorantGameMode.h"

#include "ValorantGameInstance.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "GameFramework/GameStateBase.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/BaseWeapon.h"

AValorantGameMode::AValorantGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character

	// KBD: 폴더링 하는 동안 불러오지 않도록 함
	// static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	// DefaultPawnClass = PlayerPawnClassFinder.Class;
}

void AValorantGameMode::RespawnAllPlayers()
{
	AGameStateBase* gs = GetGameState<AGameStateBase>();
	if (!gs)
	{
		UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, GS NULL"));
		return;
	}
	
	for (APlayerState* basePS : gs->PlayerArray)
	{
		AAgentPlayerState* PS = Cast<AAgentPlayerState>(basePS);
		if (!PS)
		{
			UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, PS NULL"));
			continue;
		}

		ResetAgentAtrributeData(PS);
		RespawnPlayer(PS);
	}
}

void AValorantGameMode::RespawnPlayer(AAgentPlayerState* ps)
{
	APlayerController* pc = Cast<APlayerController>(ps->GetOwner());
	if (!pc)
	{
		UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, PC NULL"));
		return;
	}
		
	FAgentData* agentData = m_GameInstance->GetAgentData(ps->GetAgentID());
		
	// TODO: 팀별 스폰 위치 가져오기
	FVector spawnLoc = RespawnLocation;
	FRotator spawnRot = RespawnRotation;
		
	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABaseAgent* newAgent = GetWorld()->SpawnActor<ABaseAgent>(agentData->AgentAsset, spawnLoc, spawnRot, params);
		
	if (!newAgent)
	{
		UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, AGENT NULL"));
		return;
	}

	if (ps->IsSpectator())
	{
		ps->SetIsSpectator(false);
		ps->SetIsOnlyASpectator(false);
	}

	APawn* oldPawn = pc->GetPawn();
		
	pc->UnPossess();
	pc->Possess(newAgent);
		
	if (oldPawn)
	{
		oldPawn->Destroy();
	}
}

void AValorantGameMode::GiveMeleeKnife()
{
	auto gs = GetGameState<AGameStateBase>();
	
	if (MeleeAsset == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::GiveMeleeKnife, MeleeAsset NULL"));
		return;
	}

	for (APlayerState* ps : gs->PlayerArray) {
		{
			if (auto* agent = Cast<ABaseAgent>(ps->GetPawn()))
			{
				ABaseWeapon* knife = GetWorld()->SpawnActor<ABaseWeapon>(MeleeAsset);
				agent->ServerRPC_Interact(knife);
			}
		}
	}
}

void AValorantGameMode::BeginPlay()
{
	Super::BeginPlay();
	m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
}

void AValorantGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void AValorantGameMode::ResetAgentAtrributeData(AAgentPlayerState* ps)
{
	ps->GetBaseAttributeSet()->ResetAttributeData();
}
