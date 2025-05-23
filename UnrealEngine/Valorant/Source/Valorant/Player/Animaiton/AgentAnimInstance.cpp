// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Agent/BaseAgent.h"

void UAgentAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	OwnerAgent = Cast<ABaseAgent>(GetOwningActor());
	if (OwnerAgent)
	{
		OwnerAgent->OnAgentEquip.AddDynamic(this, &UAgentAnimInstance::OnEquip);
		OwnerAgent->OnAgentFire.AddDynamic(this, &UAgentAnimInstance::OnFire);
		OwnerAgent->OnAgentReload.AddDynamic(this, &UAgentAnimInstance::OnReload);
		OwnerAgent->OnAgentDamaged.AddDynamic(this, &UAgentAnimInstance::OnDamaged);
		OwnerAgent->OnSpikeActive.AddDynamic(this, &UAgentAnimInstance::OnSpikeActive);
		OwnerAgent->OnSpikeCancel.AddDynamic(this, &UAgentAnimInstance::OnSpikeCancel);
		OwnerAgent->OnSpikeDeactive.AddDynamic(this, &UAgentAnimInstance::OnSpikeDeactive);
		OwnerAgent->OnSpikeDefuseFinish.AddDynamic(this, &UAgentAnimInstance::OnSpikeDefuseFinish);
	}
}

void UAgentAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	UpdateState();
}

void UAgentAnimInstance::UpdateState()
{
	if (OwnerAgent)
	{
		FVector velocity = OwnerAgent->GetVelocity();
		FVector forward = OwnerAgent->GetActorForwardVector();
		Speed = FVector::DotProduct(velocity, forward);
		FVector right = OwnerAgent->GetActorRightVector();
		Direction = FVector::DotProduct(velocity, right);

		Pitch = FRotator::NormalizeAxis(OwnerAgent->ReplicatedControlRotation.Pitch);
		// Yaw = FRotator::NormalizeAxis(OwnerAgent->ReplicatedControlRotation.Yaw);
		// NET_LOG(LogTemp, Warning, TEXT("Pitch : %f, Yaw : %f"), Pitch, Yaw);
		
		bIsCrouch = OwnerAgent->bIsCrouched;
		bIsInAir = OwnerAgent->GetCharacterMovement()->IsFalling();
		bIsDead = OwnerAgent->IsDead();
		InteractorPoseIdx = OwnerAgent->GetPoseIdx();
		
		if (InteractorState != OwnerAgent->GetInteractorState())
		{
			InteractorState = OwnerAgent->GetInteractorState();
			OnChangedWeaponState();
		}
	}
}
