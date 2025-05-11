// Fill out your copyright notice in the Description page of Project Settings.


#include "Spike.h"

#include "Components/WidgetComponent.h"
#include "GameManager/MatchGameMode.h"
#include "Player/Agent/BaseAgent.h"
#include "UI/DetectWidget.h"


ASpike::ASpike()
{
	PrimaryActorTick.bCanEverTick = true;
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh> SpikeMeshObj(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Spike/Spike.Spike'"));
	if (SpikeMeshObj.Succeeded())
	{
		Mesh->SetSkeletalMesh(SpikeMeshObj.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.34f));
}

void ASpike::BeginPlay()
{
	Super::BeginPlay();

	if (const auto* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		DetectWidget->SetName(TEXT("획득 스파이크"));
	}
}

void ASpike::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpike::ServerRPC_PickUp(ABaseAgent* Agent)
{
	Super::ServerRPC_PickUp(Agent);
}

void ASpike::ServerRPC_Drop()
{
	Super::ServerRPC_Drop();
}

void ASpike::ServerRPC_Interact(ABaseAgent* InteractAgent)
{
	Super::ServerRPC_Interact(InteractAgent);
}

bool ASpike::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	const auto* PS = Agent->GetPlayerState<AAgentPlayerState>();
	if (nullptr == PS)
	{
		return false;
	}
	return AMatchGameMode::IsAttacker(PS->bIsBlueTeam);
}

bool ASpike::ServerOnly_CanDrop() const
{
	return true;
}

bool ASpike::ServerOnly_CanInteract() const
{
	return ServerOnly_CanAutoPickUp(OwnerAgent);
}