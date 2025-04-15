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
#include "Valorant/Player/AgentPlayerState.h"


class AAgentPlayerState;

ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(GetRootComponent());
	
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ThirdPersonMesh");
	ThirdPersonMesh->SetupAttachment(Camera);
	
	ThirdPersonMesh->AlwaysLoadOnClient = true;
	ThirdPersonMesh->AlwaysLoadOnServer = true;
	ThirdPersonMesh->bOwnerNoSee = false;
	ThirdPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	ThirdPersonMesh->bCastDynamicShadow = true;
	ThirdPersonMesh->bAffectDynamicIndirectLighting = true;
	ThirdPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ThirdPersonMesh->SetGenerateOverlapEvents(false);
	ThirdPersonMesh->SetCanEverAffectNavigation(false);

	GetMesh()->SetupAttachment(Camera);

	//MovementComponent = CreateDefaultSubobject<UAgentInputComponent>("MovementComponent");
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled())
	{
		GetMesh()->SetOwnerNoSee(true);
		ThirdPersonMesh->SetOwnerNoSee(false);
		
		m_GameInstance = GetGameInstance<UValorantGameInstance>();
	}
	else
	{
		GetMesh()->SetOwnerNoSee(false);
		ThirdPersonMesh->SetOwnerNoSee(true);
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

