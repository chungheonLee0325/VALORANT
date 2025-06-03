// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/AgentPlayerController.h"
#include "Player/Agent/BaseAgent.h"


UAgentInputComponent::UAgentInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UAgentInputComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Agent = Cast<ABaseAgent>(GetOwner());

	if (!Agent->IsLocallyControlled())
	{
		return;
	}
	
	if (APlayerController* pc = Cast<APlayerController>(Agent->GetInstigatorController()))
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
	// NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	if (auto* eic = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (LeftMouseStartAction && LeftMouseEndAction)
		{
			eic->BindAction(LeftMouseStartAction,ETriggerEvent::Triggered, this, &UAgentInputComponent::StartFire);
			eic->BindAction(LeftMouseEndAction,ETriggerEvent::Triggered, this, &UAgentInputComponent::EndFire);
		}
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
		
		if (Num_1Action && Num_2Action && Num_3Action && Num_4Action)
		{
			eic->BindAction(Num_1Action, ETriggerEvent::Started, this, &UAgentInputComponent::Weapon1);
			eic->BindAction(Num_2Action, ETriggerEvent::Started, this, &UAgentInputComponent::Weapon2);
			eic->BindAction(Num_3Action, ETriggerEvent::Started, this, &UAgentInputComponent::Weapon3);
			eic->BindAction(Num_4Action, ETriggerEvent::Started, this, &UAgentInputComponent::Weapon4);
			eic->BindAction(Num_4Action, ETriggerEvent::Completed, this, &UAgentInputComponent::Weapon4Released);
		}
		if (ReloadAction)
		{
			eic->BindAction(ReloadAction, ETriggerEvent::Started, this, &UAgentInputComponent::StartReload);
		}

		if (InteractAction)
		{
			eic->BindAction(InteractAction, ETriggerEvent::Started, this, &UAgentInputComponent::Interact);
		}
		if (DropAction)
		{
			eic->BindAction(DropAction, ETriggerEvent::Started, this, &UAgentInputComponent::Drop);
		}
		
		if (ShopUIAction)
		{
			eic->BindAction(ShopUIAction, ETriggerEvent::Started, this, &UAgentInputComponent::ShopUI);
		}
	}
}

void UAgentInputComponent::OnMove(const FInputActionValue& value)
{
	if (Agent->CanMove())
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

void UAgentInputComponent::StartFire(const FInputActionValue& InputActionValue)
{
	if (const auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		if (PC->ShopUI)
		{
			// 상점 열려있으면 쏘지마라
			return;
		}
	}
	
	if (!Agent->IsDead())
	{
		// 스킬을 실행했다면, 리턴
		if (Agent->GetASC()->TryActivateAbilityByTag(LeftClickTag))
		{
			return;
		}
		
		// NET_LOG(LogTemp, Warning, TEXT("파이어 시도"));
		Agent->StartFire();
	}
}

void UAgentInputComponent::EndFire(const FInputActionValue& InputActionValue)
{
	if (const auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		if (PC->ShopUI)
		{
			// 상점 열려있으면 쏘지마라
			return;
		}
	}
	
	if (!Agent->IsDead())
	{
		// NET_LOG(LogTemp, Warning, TEXT("파이어 종료"));
		Agent->EndFire();
	}
}

void UAgentInputComponent::JumpStart(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		if (Agent->bIsCrouched)
		{
			Agent->UnCrouch();
			// UE_LOG(LogTemp, Warning, TEXT("앉기 중 점프"));
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
	if (!Agent->IsDead())
	{
		Agent->Crouch();
	}
}

void UAgentInputComponent::CrouchComplete(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->UnCrouch();
	}
}

void UAgentInputComponent::WalkStart(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
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

void UAgentInputComponent::Drop(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->ServerRPC_DropCurrentInteractor();
	}
}

void UAgentInputComponent::Interact(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->Interact();
	}
}

void UAgentInputComponent::WeaponChange(const FInputActionValue& value)
{
	//TODO: Enum int 로 변환하여 휠으로 슬롯 체인지
}

void UAgentInputComponent::Weapon1(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->SwitchEquipment(EInteractorType::MainWeapon);
	}
}

void UAgentInputComponent::Weapon2(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->SwitchEquipment(EInteractorType::SubWeapon);
	}
}

void UAgentInputComponent::Weapon3(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->SwitchEquipment(EInteractorType::Melee);
	}
}

void UAgentInputComponent::Weapon4(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->ActivateSpike();
	}
}

void UAgentInputComponent::Weapon4Released(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->CancelSpike(nullptr);
	}
}

void UAgentInputComponent::StartReload(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		Agent->Reload();
	}
}

void UAgentInputComponent::ShopUI(const FInputActionValue& InputActionValue)
{
	if (Agent)
	{
		Agent->SetShopUI();
	}
}
