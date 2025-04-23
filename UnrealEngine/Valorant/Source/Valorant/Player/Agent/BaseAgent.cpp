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
#include "Player/Component/AgentInputComponent.h"
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "Valorant/Player/Widget/AgentBaseWidget.h"


class AAgentPlayerState;

ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(-10, 0, 60));
	SpringArm->TargetArmLength = 0;
	SpringArm->bUsePawnControlRotation = true;
	
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	
	GetMesh()->SetupAttachment(SpringArm);
	GetMesh()->SetRelativeScale3D(FVector(.34f));
	GetMesh()->SetRelativeLocation(FVector(10,0,-155));
	
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
	GetCharacterMovement()->MaxWalkSpeedCrouched = 250.0f;
	GetCharacterMovement()->SetCrouchedHalfHeight(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());

	ThirdPersonMesh->SetOwnerNoSee(true);
	GetMesh()->SetOnlyOwnerSee(true);

	AgentInputComponent = CreateDefaultSubobject<UAgentInputComponent>("InputComponent");
}

// 서버 전용. 캐릭터를 Possess할 때 호출됨. 게임 첫 시작시, BeginPlay 보다 먼저 호출됩니다.
void ABaseAgent::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//국룰 위치
	InitAgentData();
	
	AAgentPlayerController* pc = Cast<AAgentPlayerController>(NewController);
	if (pc)
	{
		BindToDelegatePC(pc);
	}
}

// 클라이언트 전용. 서버로부터 PlayerState를 최초로 받을 때 호출됨
void ABaseAgent::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (IsLocallyControlled())
	{
		//국룰 위치
		InitAgentData();
		
		AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetController());
		if (pc)
		{
			// UE_LOG(LogTemp, Warning, TEXT("클라, 델리게이트 바인딩"));
			BindToDelegatePC(pc);
		}
	}
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();
	PS = GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		GetCharacterMovement()->MaxWalkSpeed = PS->GetMoveSpeed();
	}
}

void ABaseAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseAgent::InitAgentData()
{
	AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>();
	if (ps == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerState를 AAgentPlayerState를 상속받는 녀석으로 교체 부탁"));
		return;
	}
	
	ASC = Cast<UAgentAbilitySystemComponent>(ps->GetAbilitySystemComponent());
	ASC->InitAbilityActorInfo(ps, this);
	
	if (HasAuthority())
	{
		// 스킬 등록 및 값 초기화는 서버에서만 진행
		ASC->InitializeByAgentData(m_AgentID);
		
		// UE_LOG(LogTemp, Warning, TEXT("=== ASC 등록된 GA 목록 ==="));
		// for (const FGameplayAbilitySpec& spec : ASC->GetActivatableAbilities())
		// {
		// 	if (spec.Ability)
		// 	{
		// 		UE_LOG(LogTemp, Warning, TEXT("GA: %s"), *spec.Ability->GetName());
		//
		// 		FString TagString;
		// 		TArray<FGameplayTag> tags = spec.GetDynamicSpecSourceTags().GetGameplayTagArray();
		// 		for (const FGameplayTag& Tag : tags)
		// 		{
		// 			TagString += Tag.ToString() + TEXT(" ");
		// 		}
		//
		// 		UE_LOG(LogTemp, Warning, TEXT("태그 목록: %s"), *TagString);
		// 	}
		// }
	}
}

void ABaseAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	AgentInputComponent->BindInput(PlayerInputComponent);
}

void ABaseAgent::BindToDelegatePC(AAgentPlayerController* pc)
{
	pc->OnHealthChanged_PC.AddDynamic(this, &ABaseAgent::UpdateHealth);
	pc->OnMaxHealthChanged_PC.AddDynamic(this, &ABaseAgent::UpdateMaxHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &ABaseAgent::UpdateArmor);
	pc->OnMoveSpeedChanged_PC.AddDynamic(this, &ABaseAgent::UpdateMoveSpeed);
}

void ABaseAgent::Die()
{
	bIsDead = true;
	UE_LOG(LogTemp, Warning, TEXT("죽음"));
}

void ABaseAgent::EnterSpectMode()
{
}

void ABaseAgent::Respawn()
{
}

void ABaseAgent::UpdateHealth(float newHealth)
{
	if (newHealth <= 0.f && bIsDead == false)
	{
		Die();
	}
}

void ABaseAgent::UpdateMaxHealth(float newMaxHealth)
{
}

void ABaseAgent::UpdateArmor(float newArmor)
{
}

void ABaseAgent::UpdateMoveSpeed(float newSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = newSpeed;
}
