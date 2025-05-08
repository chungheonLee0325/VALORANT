// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValorantCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Weapon/ValorantPickUpComponent.h"
#include "ValorantWeaponComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AValorantCharacter

AValorantCharacter::AValorantCharacter()
{
	// KBD: 반동 회복을 위해 Tick 활성화
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	InteractionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionCapsule"));
	InteractionCapsule->SetupAttachment(FirstPersonCameraComponent);
	InteractionCapsule->SetRelativeLocation(FVector(150, 0, 0));
	InteractionCapsule->SetRelativeRotation(FRotator(-90, 0, 0));
	InteractionCapsule->SetCapsuleHalfHeight(150);
	InteractionCapsule->SetCapsuleRadius(35);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
}

void AValorantCharacter::BeginPlay()
{
	Super::BeginPlay();

	InteractionCapsule->OnComponentBeginOverlap.AddDynamic(this, &AValorantCharacter::OnFindInteraction);
	InteractionCapsule->OnComponentEndOverlap.AddDynamic(this, &AValorantCharacter::OnInteractionCapsuleEndOverlap);
}

//////////////////////////////////////////////////////////////////////////// Input

void AValorantCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AValorantCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AValorantCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AValorantCharacter::Look);

		EnhancedInputComponent->BindAction(InteractionAction, ETriggerEvent::Triggered, this, &AValorantCharacter::Interaction);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AValorantCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AValorantCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AValorantCharacter::Interaction()
{
	if (FindPickUpComponent)
	{
		TArray<USceneComponent*> ChildrenArray;
		GetMesh1P()->GetChildrenComponents(true, ChildrenArray);
		for (auto* Child : ChildrenArray)
		{
			if (auto* Weapon = Cast<UValorantWeaponComponent>(Child))
			{
				Weapon->Drop();
				break;
			}
		}
		
		// FindPickUpComponent->PickUp(this);
		FindPickUpComponent = nullptr;
	}
}

void AValorantCharacter::OnFindInteraction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// TODO: 애초에 PickUp Component만 감지되게 Collision 설정
	if (auto* PickUpComponent = Cast<UValorantPickUpComponent>(OtherComp))
	{
		if (PickUpComponent)
		{
			if (auto* NewPickUpUI = OtherActor->FindComponentByClass<UWidgetComponent>())
			{
				NewPickUpUI->SetVisibility(true);
			}
		}
		FindPickUpComponent = PickUpComponent;
	}
}

void AValorantCharacter::OnInteractionCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const auto* PickUpComponent = Cast<UValorantPickUpComponent>(OtherComp))
	{
		if (PickUpComponent == FindPickUpComponent)
		{
			if (auto* PrevPickUpUI = FindPickUpComponent->GetOwner()->FindComponentByClass<UWidgetComponent>())
			{
				PrevPickUpUI->SetVisibility(false);
			}
			FindPickUpComponent = nullptr;
		}
	}
}

void AValorantCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// KBD: 현재 뭔가를 발사 중이 아니고, 누적반동값이 0이 아니라면 반동을 회복하자
	if (false == bIsFiring && TotalRecoilOffsetPitch + TotalRecoilOffsetYaw != 0.0f)
	{
		const float SubPitchValue = -FMath::Lerp(TotalRecoilOffsetPitch, 0.0f, 0.88f);
		TotalRecoilOffsetPitch += SubPitchValue;
		AddControllerPitchInput(SubPitchValue);

		const float SubYawValue = -FMath::Lerp(TotalRecoilOffsetYaw, 0.0f, 0.88f);
		TotalRecoilOffsetYaw += SubYawValue;
		AddControllerYawInput(SubYawValue);
	}
}
