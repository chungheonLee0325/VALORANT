// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/Agent/BaseAgent.h"


UAgentInputComponent::UAgentInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UAgentInputComponent::BeginPlay()
{
	Super::BeginPlay();

	Agent = Cast<ABaseAgent>(GetOwner());
	if (Agent)
	{
		UE_LOG(LogTemp, Warning, TEXT("InputComp, Get Agent: %s"), *Agent->GetName());
	}
	
	if (APlayerController* pc = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
	{
		if (ULocalPlayer* player = pc->GetLocalPlayer())
		{
			InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(player);
			if (InputSubsystem && DefaultMappingContext)
			{
				InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void UAgentInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAgentInputComponent::BindInput(UInputComponent* InputComponent)
{
	if (auto* eic = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
			eic->BindAction(MoveAction,ETriggerEvent::Triggered, this, &UAgentInputComponent::OnMove);

		if (LookAction)
			eic->BindAction(LookAction, ETriggerEvent::Triggered, this, &UAgentInputComponent::OnLook);
		
		if (JumpAction)
		{
			eic->BindAction(JumpAction, ETriggerEvent::Started, this, &UAgentInputComponent::JumpStart);
			eic->BindAction(JumpAction, ETriggerEvent::Started, this, &UAgentInputComponent::JumpComplete);
		}

		if (ShiftAction)
		{
			eic->BindAction(ShiftAction, ETriggerEvent::Started, this, &UAgentInputComponent::WalkStart);
			eic->BindAction(ShiftAction, ETriggerEvent::Completed, this, &UAgentInputComponent::WalkComplete);
		}

		if (CtrlAction)
		{
			eic->BindAction(CtrlAction, ETriggerEvent::Started, this, &UAgentInputComponent::CrouchStart);
			eic->BindAction(CtrlAction, ETriggerEvent::Completed, this, &UAgentInputComponent::CrouchComplete);
		}
	}
}

void UAgentInputComponent::OnMove(const FInputActionValue& value)
{
	if (Agent)
	{
		const FVector2D& moveVector = value.Get<FVector2D>();
		Agent->AddMovementInput(Agent->GetActorForwardVector(), moveVector.Y);
		Agent->AddMovementInput(Agent->GetActorRightVector(), moveVector.X);
	}
}

void UAgentInputComponent::OnLook(const FInputActionValue& value)
{
	if (Agent)
	{
		const FVector2D& lookVector = value.Get<FVector2D>();
		Agent->AddControllerYawInput(lookVector.X);
		Agent->AddControllerPitchInput(lookVector.Y);
	}
}

void UAgentInputComponent::JumpStart(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		if (Agent->bIsCrouched)
		{
			Agent->UnCrouch();
			UE_LOG(LogTemp, Warning, TEXT("앉기 중 점프"));
		}
		
		Agent->Jump();
	}
}

void UAgentInputComponent::JumpComplete(const FInputActionValue& InputActionValue)
{
	// if (Agent)
	// {
	// 	Agent->StopJumping();
	// }
}

void UAgentInputComponent::CrouchStart(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->Crouch();
	}
}

void UAgentInputComponent::CrouchComplete(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->UnCrouch();
	}
}

void UAgentInputComponent::WalkStart(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->SetIsRun(false);
	}
}

void UAgentInputComponent::WalkComplete(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->SetIsRun(true);
	}
}
