// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValorantGameMode.h"

#include "EngineUtils.h"
#include "ValorantGameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"
#include "Valorant/ValorantCharacter.h"
#include "UObject/ConstructorHelpers.h"

AValorantGameMode::AValorantGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character

	// KBD: 폴더링 하는 동안 불러오지 않도록 함
	// static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	// DefaultPawnClass = PlayerPawnClassFinder.Class;
}

void AValorantGameMode::RespawnAllPlayer()
{
	AGameStateBase* gs = GetGameState<AGameStateBase>();
	if (!gs)
	{
		UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, GS NULL"));
		return;
	}
	
	
	for (APlayerState* basePS : gs->PlayerArray)
	{
		AAgentPlayerState* ps = Cast<AAgentPlayerState>(basePS);
		if (!ps)
		{
			UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, PS NULL"));
			continue;
		}
		
		FAgentData* agentData = m_GameInstance->GetAgentData(ps->GetAgentID());

		// TODO: 팀별 스폰 위치 가져오기
		FVector spawnLoc = FVector::ZeroVector;
		FRotator spawnRot = FRotator::ZeroRotator;
		
		ABaseAgent* newAgent = GetWorld()->SpawnActor<ABaseAgent>(agentData->AgentAsset, spawnLoc, spawnRot);

		
		if (newAgent)
		{
			APlayerController* pc = nullptr;

			// 죽어서 관전자로 전환된 경우, Owner가 없으므로 찾아서 매칭
			if (!ps->GetOwner())
			{
				for (AController* controller : TActorRange<AController>(GetWorld()))
				{
					AAgentPlayerController* tmpPC = Cast<AAgentPlayerController>(controller);
					if (tmpPC && tmpPC->PlayerState == ps)
					{
						pc = tmpPC;
						break;
					}
				}
			}
			else
			{
				pc = Cast<APlayerController>(ps->GetOwner());
			}
		
			if (!pc)
			{
				UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, PC NULL"));
				return;
			}
			
			pc->Possess(newAgent);
		}
		else
		{
			UE_LOG(LogTemp,Error,TEXT("AValorantGameMode::RespawnAllPlayer, AGENT or OWNER NULL"));
		}
	}
}


void AValorantGameMode::BeginPlay()
{
	Super::BeginPlay();
	m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
}
