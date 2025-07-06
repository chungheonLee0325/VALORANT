// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
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
		
		// 우클릭 바인딩 추가
		if (RightMouseAction)
		{
			eic->BindAction(RightMouseAction, ETriggerEvent::Started, this, &UAgentInputComponent::StartRightClick);
			eic->BindAction(RightMouseAction, ETriggerEvent::Completed, this, &UAgentInputComponent::EndRightClick);
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

		if (CAction)
		{
			eic->BindAction(CAction, ETriggerEvent::Started, this, &UAgentInputComponent::AbilityCInput);
		}
		if (QAction)
		{
			eic->BindAction(QAction, ETriggerEvent::Started, this, &UAgentInputComponent::AbilityQInput);
		}
		if (EAction)
		{
			eic->BindAction(EAction, ETriggerEvent::Started, this, &UAgentInputComponent::AbilityEInput);
		}
		if (XAction)
		{
			eic->BindAction(XAction, ETriggerEvent::Started, this, &UAgentInputComponent::AbilityXInput);
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
	// 상점 UI가 열려있으면 무시
	if (const auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		if (PC->ShopUI)
		{
			return;
		}
	}
	
	if (Agent->IsDead())
	{
		return;
	}

	// ASC 가져오기
	UAgentAbilitySystemComponent* ASC = Agent->GetASC();
	if (!ASC)
	{
		return;
	}

	// 기본 좌클릭 어빌리티 시도 (일반적으로 무기 발사)
	if (!ASC->TryActivateAbilityByTag(LeftClickTag))
	{
		// 어빌리티가 없으면 일반 발사
		Agent->StartFire();
	}
}

void UAgentInputComponent::EndFire(const FInputActionValue& InputActionValue)
{
	if (const auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		if (PC->ShopUI)
		{
			return;
		}
	}
	
	if (!Agent->IsDead())
	{
		Agent->EndFire();
	}
}

void UAgentInputComponent::StartRightClick(const FInputActionValue& InputActionValue)
{
	// 상점 UI가 열려있으면 무시
	if (const auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		if (PC->ShopUI)
		{
			return;
		}
	}
	
	if (Agent->IsDead())
	{
		return;
	}

	// ASC 가져오기
	UAgentAbilitySystemComponent* ASC = Agent->GetASC();
	if (!ASC)
	{
		return;
	}

	// 기본 우클릭 시도
	if (!ASC->TryActivateAbilityByTag(RightClickTag))
	{
		// 어빌리티가 없으면 Agent의 기본 우클릭 동작 실행
	}
}

void UAgentInputComponent::EndRightClick(const FInputActionValue& InputActionValue)
{
	if (const auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		if (PC->ShopUI)
		{
			return;
		}
	}
	
	if (!Agent->IsDead())
	{
		// Agent에 우클릭 종료 처리 함수가 있다면 호출
		// 예: Agent->EndAiming();
	}
}

void UAgentInputComponent::JumpStart(const FInputActionValue& InputActionValue)
{
	if (!Agent->IsDead())
	{
		if (Agent->bIsCrouched)
		{
			Agent->UnCrouch();
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

void UAgentInputComponent::AbilityCInput(const FInputActionValue& InputActionValue)
{
	Agent->GetASC()->TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
}

void UAgentInputComponent::AbilityQInput(const FInputActionValue& InputActionValue)
{
	Agent->GetASC()->TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
}

void UAgentInputComponent::AbilityEInput(const FInputActionValue& InputActionValue)
{
	Agent->GetASC()->TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
}

void UAgentInputComponent::AbilityXInput(const FInputActionValue& InputActionValue)
{
	Agent->GetASC()->TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")));
}
