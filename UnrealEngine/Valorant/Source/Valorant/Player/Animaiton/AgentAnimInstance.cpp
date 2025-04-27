// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAnimInstance.h"

#include "Valorant.h"
#include "GameFramework/CharacterMovementComponent.h"
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

		// FRotator rot = player->GetBaseAimRotation();
		// Pitch = rot.Pitch;
		// Yaw = rot.Yaw;
		// if (player->IsLocallyControlled() == false)
		// {
		// 	UE_LOG(LogTemp, Warning, TEXT("Pitch %f"),Pitch);
		// 	UE_LOG(LogTemp, Warning, TEXT("Yaw %f"),Yaw);
		// 	
		// }
	

		// UE_LOG(LogTemp, Warning, TEXT("Speed %f"),Speed);
		// UE_LOG(LogTemp, Warning, TEXT("Dir %f"),Direction);
		
		bIsCrouch = player->bIsCrouched;
		bIsInAir = player->GetCharacterMovement()->IsFalling();
	}
}
