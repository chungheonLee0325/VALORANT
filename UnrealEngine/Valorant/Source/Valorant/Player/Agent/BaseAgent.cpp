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
	
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	
	GetMesh()->SetupAttachment(SpringArm);
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

// 서버 전용. 캐릭터를 Possess할 때 호출됨. 게임 첫 시작시, BeginPlay 보다 먼저 호출됩니다.
void ABaseAgent::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAgentData();
}

// 클라이언트 전용. 서버로부터 PlayerState를 최초로 받을 때 호출됨
void ABaseAgent::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAgentData();
	InitUI();
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitUI();
	}
	
	if (IsLocallyControlled())
	{
		//UE_LOG(LogTemp, Warning, TEXT("내 캐릭터"));
		// GetMesh()->SetOwnerNoSee(false);
		// ThirdPersonMesh->SetOwnerNoSee(true);
		
		ThirdPersonMesh->SetVisibility(false);
		GetMesh()->SetVisibility(true);
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("다른 사람 캐릭터"));
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
		ASC->InitializeByAgentData(m_AgentID);
		
		UE_LOG(LogTemp, Warning, TEXT("=== ASC 등록된 GA 목록 ==="));
		for (const FGameplayAbilitySpec& spec : ASC->GetActivatableAbilities())
		{
			if (spec.Ability)
			{
				UE_LOG(LogTemp, Warning, TEXT("GA: %s"), *spec.Ability->GetName());

				FString TagString;
				TArray<FGameplayTag> tags = spec.GetDynamicSpecSourceTags().GetGameplayTagArray();
				for (const FGameplayTag& Tag : tags)
				{
					TagString += Tag.ToString() + TEXT(" ");
				}

				UE_LOG(LogTemp, Warning, TEXT("태그 목록: %s"), *TagString);
			}
		}
	}
}

void ABaseAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseAgent::Crouch(bool bClientSimulation)
{
	Super::Crouch(bClientSimulation);
	
}

void ABaseAgent::InitUI()
{
	if (IsLocallyControlled())
	{
		AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetController<AAgentPlayerController>());
		AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>();
		if (pc && ps)
		{
			pc->InitUI(ps->GetHealth(),ps->GetArmor(),ps->GetMoveSpeed());
		}
	}
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
