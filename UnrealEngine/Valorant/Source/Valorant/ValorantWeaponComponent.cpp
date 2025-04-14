// Copyright Epic Games, Inc. All Rights Reserved.


#include "ValorantWeaponComponent.h"
#include "ValorantCharacter.h"
#include "ValorantProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "ResourceManager/ValorantGameType.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameManager/ValorantGameInstance.h"

// Sets default values for this component's properties
UValorantWeaponComponent::UValorantWeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}


void UValorantWeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	Character->bIsFiring = true;
	if (FMath::IsNearlyZero(Character->TotalRecoilOffsetPitch + Character->TotalRecoilOffsetYaw, 0.1f))
	{
		Character->TotalRecoilOffsetPitch = 0.0f;
		Character->TotalRecoilOffsetYaw = 0.0f;
		RecoilLevel = 0;
	}

	UE_LOG(LogTemp, Warning, TEXT("RecoilLevel : %d"), RecoilLevel);
	
	// KBD: 발사 시 캐릭터에 반동값 적용
	const float PitchValue = RecoilData[RecoilLevel].OffsetPitch * 0.2;
	UE_LOG(LogTemp, Warning, TEXT("PitchValue : %f"), PitchValue);
	Character->AddControllerPitchInput(PitchValue);
	Character->TotalRecoilOffsetPitch += PitchValue;

	const float YawValue = RecoilData[RecoilLevel].OffsetYaw * 0.2;
	UE_LOG(LogTemp, Warning, TEXT("YawValue : %f"), YawValue);
	Character->AddControllerYawInput(YawValue);
	Character->TotalRecoilOffsetYaw += YawValue;
	
	RecoilLevel = FMath::Clamp(RecoilLevel + 1, 0, RecoilData.Num() - 1);

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			const APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			FVector Start, Dir;
			int32 ScreenWidth, ScreenHeight;
			PlayerController->GetViewportSize(ScreenWidth, ScreenHeight);
			PlayerController->DeprojectScreenPositionToWorld(ScreenWidth * 0.5f, ScreenHeight * 0.5f, Start, Dir);
			const FVector End = Start + Dir * 9999;
			
			DrawDebugLine(World, Start, End, FColor::Red, false, 3, 0, 0.3);
			
			// const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// // MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			// const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
			//
			// //Set Spawn Collision Handling Override
			// FActorSpawnParameters ActorSpawnParams;
			// ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
			//
			// // Spawn the projectile at the muzzle
			// World->SpawnActor<AValorantProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void UValorantWeaponComponent::EndFire()
{
	if (nullptr == Character)
	{
		return;
	}
	
	Character->bIsFiring = false;
}

bool UValorantWeaponComponent::AttachWeapon(AValorantCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon component yet
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<UValorantWeaponComponent>())
	{
		return false;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UValorantWeaponComponent::Fire);
			EnhancedInputComponent->BindAction(EndFireAction, ETriggerEvent::Triggered, this, &UValorantWeaponComponent::EndFire);
		}
	}

	return true;
}

void UValorantWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ensure we have a character owner
	if (Character != nullptr)
	{
		// remove the input mapping context from the Player Controller
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}
		}
	}

	// maintain the EndPlay call chain
	Super::EndPlay(EndPlayReason);
}

void UValorantWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	auto* GameInstance = Cast<UValorantGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInstance)
	{
		auto* WeaponData = GameInstance->GetWeaponData(1);
		if (WeaponData)
		{
			for (auto Element : WeaponData->GunRecoilMap)
			{
				RecoilData.Add(Element);
			}
		}
	}
	
}
