// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAnimInstance.h"

#include "Valorant.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Net/UnrealNetwork.h"
#include "Player/Agent/BaseAgent.h"

void UAgentAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	auto ownerPawn = TryGetPawnOwner();
	
	auto player = Cast<ABaseAgent>(ownerPawn);
	if (player)
	{
		FVector velocity = player->GetVelocity();

		FVector forward = player->GetActorForwardVector();
		Speed = FVector::DotProduct(velocity, forward);
		FVector right = player->GetActorRightVector();
		Direction = FVector::DotProduct(velocity, right);
		
		bIsCrouch = player->bIsCrouched;
		bIsInAir = player->GetCharacterMovement()->IsFalling();
	}
}
