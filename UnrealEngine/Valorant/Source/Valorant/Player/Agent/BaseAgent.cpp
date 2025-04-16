// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
//#include "Valorant/Player/AgentInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "Valorant/Player/Widget/AgentBaseWidget.h"


class AAgentPlayerState;

ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(-10, 0, 60));
	
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	
	GetMesh()->SetupAttachment(GetRootComponent());
	GetMesh()->SetRelativeScale3D(FVector(.34f));
	GetMesh()->SetRelativeLocation(FVector(-30,0,-90));
	
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ThirdPersonMesh");
	ThirdPersonMesh->SetupAttachment(GetRootComponent());
	ThirdPersonMesh->SetRelativeScale3D(FVector(.34f));
	ThirdPersonMesh->SetRelativeLocation(FVector(.0f,.0f,-90.f));
	
	ThirdPersonMesh->AlwaysLoadOnClient = true;
	ThirdPersonMesh->AlwaysLoadOnServer = true;
	ThirdPersonMesh->bOwnerNoSee = false;
	ThirdPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	ThirdPersonMesh->bCastDynamicShadow = true;
	ThirdPersonMesh->bAffectDynamicIndirectLighting = true;
	ThirdPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ThirdPersonMesh->SetGenerateOverlapEvents(false);
	ThirdPersonMesh->SetCanEverAffectNavigation(false);

	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->CrouchedHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	//MovementComponent = CreateDefaultSubobject<UAgentInputComponent>("MovementComponent");
}

void ABaseAgent::AddCameraYawInput(float val)
{
	float pitch = SpringArm->GetRelativeRotation().Pitch;
	float newPitch = pitch + val * RotOffset;
	SpringArm->SetRelativeRotation(FRotator(newPitch, 0, 0));
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("내 캐릭터"));
		// GetMesh()->SetOwnerNoSee(false);
		// ThirdPersonMesh->SetOwnerNoSee(true);
		
		ThirdPersonMesh->SetVisibility(false);
		GetMesh()->SetVisibility(true);
		
		m_GameInstance = GetGameInstance<UValorantGameInstance>();

		if (AgentWidgetClass == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("AgentWidget에 AgentWidget 좀 넣어주세요."));
			return;
		}
		
		APlayerController* pc = Cast<APlayerController>(GetController());
		if (pc)
		{
			AgentWidget = CreateWidget<UAgentBaseWidget>(pc, AgentWidgetClass);
			if (AgentWidget)
			{
				AgentWidget->AddToViewport();
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("다른 사람 캐릭터"));
		ThirdPersonMesh->SetVisibility(true);
		GetMesh()->SetVisibility(false);
	}

	AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>();
	if (PS && PS->GetAbilitySystemComponent())
	{
		ASC = Cast<UAgentAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute()).AddUObject(this,&ABaseAgent::OnHealthChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMaxHealthAttribute()).AddUObject(this,&ABaseAgent::OnMaxHealthChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetArmorAttribute()).AddUObject(this,&ABaseAgent::OnArmorChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMoveSpeedAttribute()).AddUObject(this,&ABaseAgent::OnMoveSpeedChanged);
	}
}

void ABaseAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



void ABaseAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseAgent::Crouch(bool bClientSimulation)
{
	Super::Crouch(bClientSimulation);
	
}

// 값 변경시 콜백함수
void ABaseAgent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	float health = Data.NewValue;
}

void ABaseAgent::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	float maxhealth = Data.NewValue;
}

void ABaseAgent::OnArmorChanged(const FOnAttributeChangeData& Data)
{
	float armor = Data.NewValue;
}

void ABaseAgent::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	float movespeed = Data.NewValue;
	GetCharacterMovement()->MaxWalkSpeed = movespeed;
}
