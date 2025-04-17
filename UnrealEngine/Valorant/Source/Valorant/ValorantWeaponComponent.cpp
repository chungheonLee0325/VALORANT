// Copyright Epic Games, Inc. All Rights Reserved.


#include "ValorantWeaponComponent.h"
#include "ValorantCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "ValorantPickUpComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h"
#include "ResourceManager/ValorantGameType.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameManager/ValorantGameInstance.h"

// Sets default values for this component's properties
UValorantWeaponComponent::UValorantWeaponComponent()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshAssetObj(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/FPWeapon/Mesh/SK_FPGun.SK_FPGun'"));
	if (SkeletalMeshAssetObj.Succeeded())
	{
		SetSkeletalMeshAsset(SkeletalMeshAssetObj.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> FireSoundObj(TEXT("/Script/Engine.SoundWave'/Game/Resource/FPWeapon/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02'"));
	if (FireSoundObj.Succeeded())
	{
		FireSound = FireSoundObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireAnimationObj(TEXT("/Script/Engine.AnimMontage'/Game/Resource/FirstPersonArms/Animations/FP_Rifle_Shoot_Montage.FP_Rifle_Shoot_Montage'"));
	if (FireAnimationObj.Succeeded())
	{
		FireAnimation = FireAnimationObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> FireMappingContextObj(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/BluePrint/FirstPerson/Input/IMC_Weapons.IMC_Weapons'"));
	if (FireMappingContextObj.Succeeded())
	{
		FireMappingContext = FireMappingContextObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> StartFireActionObj(TEXT("/Script/EnhancedInput.InputAction'/Game/BluePrint/FirstPerson/Input/Actions/IA_StartFire.IA_StartFire'"));
	if (StartFireActionObj.Succeeded())
	{
		StartFireAction = StartFireActionObj.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<UInputAction> EndFireActionObj(TEXT("/Script/EnhancedInput.InputAction'/Game/BluePrint/FirstPerson/Input/Actions/IA_EndFire.IA_EndFire'"));
	if (EndFireActionObj.Succeeded())
	{
		EndFireAction = EndFireActionObj.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<UInputAction> StartReloadActionObj(TEXT("/Script/EnhancedInput.InputAction'/Game/BluePrint/FirstPerson/Input/Actions/IA_StartReload.IA_StartReload'"));
	if (StartReloadActionObj.Succeeded())
	{
		StartReloadAction = StartReloadActionObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DropActionObj(TEXT("/Script/EnhancedInput.InputAction'/Game/BluePrint/FirstPerson/Input/Actions/IA_Drop.IA_Drop'"));
	if (DropActionObj.Succeeded())
	{
		DropAction = DropActionObj.Object;
	}
}

void UValorantWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	if (auto* GameInstance = Cast<UValorantGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
	{
		if (auto* Data = GameInstance->GetWeaponData(WeaponID))
		{
			WeaponData = Data;
			MagazineSize = WeaponData->MagazineSize;
			MagazineAmmo = MagazineSize;
			// TODO: 총기별 여분탄약 데이터 추가 필요
			SpareAmmo = MagazineSize * 3;
			FireInterval = 1.0f / Data->FireRate;
			for (auto Element : Data->GunRecoilMap)
			{
				RecoilData.Add(Element);
			}
		}
	}
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

void UValorantWeaponComponent::StartFire()
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
	UE_LOG(LogTemp, Warning, TEXT("StartFire, RecoilLevel : %d"), RecoilLevel);

	GetWorld()->GetTimerManager().SetTimer(AutoFireHandle, this, &UValorantWeaponComponent::Fire, 0.01f, true, 0);
}

void UValorantWeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr || false == Character->bIsFiring)
	{
		return;
	}

	if (MagazineAmmo <= 0)
	{
		if (SpareAmmo > 0)
		{
			StartReload();
		}
		return;
	}
	
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime + FireInterval > CurrentTime)
	{
		return;
	}
	LastFireTime = CurrentTime;
	MagazineAmmo--;
	
	// KBD: 발사 시 캐릭터에 반동값 적용
	if (RecoilData.Num() > 0)
	{
		const float PitchValue = RecoilData[RecoilLevel].OffsetPitch;
		Character->AddControllerPitchInput(PitchValue);
		Character->TotalRecoilOffsetPitch += PitchValue;

		const float YawValue = RecoilData[RecoilLevel].OffsetYaw;
		Character->AddControllerYawInput(YawValue);
		Character->TotalRecoilOffsetYaw += YawValue;
		
		RecoilLevel = FMath::Clamp(RecoilLevel + 1, 0, RecoilData.Num() - 1);
		
		UE_LOG(LogTemp, Warning, TEXT("Ammo : %d, Total : (%f, %f), Add : (%f, %f)"), MagazineAmmo, Character->TotalRecoilOffsetPitch, Character->TotalRecoilOffsetYaw, PitchValue, YawValue);
	}

	if (const UWorld* const World = GetWorld())
	{
		const APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		FVector Start, Dir;
		int32 ScreenWidth, ScreenHeight;
		PlayerController->GetViewportSize(ScreenWidth, ScreenHeight);
		PlayerController->DeprojectScreenPositionToWorld(ScreenWidth * 0.5f, ScreenHeight * 0.5f, Start, Dir);
		const FVector End = Start + Dir * 9999;

		// 궤적, 탄착군 디버깅
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(GetOwner());
		FHitResult OutHit;
		const bool bHit = UKismetSystemLibrary::LineTraceSingle(
			World,
			Start,
			End,
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::ForDuration,
			OutHit,
			true
		);
		if (bHit)
		{
			DrawDebugPoint(World, OutHit.ImpactPoint, 10, FColor::Green, false, 30);
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

	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
}

bool UValorantWeaponComponent::AttachWeapon(AValorantCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	UE_LOG(LogTemp, Warning, TEXT("AttachWeapon %p"), this);
	
	// Check that the character is valid, and has no weapon component yet
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<UValorantWeaponComponent>())
	{
		return false;
	}

	if (const auto* Owner = GetOwner())
	{
		if (auto* PickUpUI = Owner->FindComponentByClass<UWidgetComponent>())
		{
			PickUpUI->SetVisibility(false);
		}
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
			EnhancedInputComponent->BindAction(StartFireAction, ETriggerEvent::Triggered, this, &UValorantWeaponComponent::StartFire);
			EnhancedInputComponent->BindAction(EndFireAction, ETriggerEvent::Triggered, this, &UValorantWeaponComponent::EndFire);
			EnhancedInputComponent->BindAction(StartReloadAction, ETriggerEvent::Triggered, this, &UValorantWeaponComponent::StartReload);
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Triggered, this, &UValorantWeaponComponent::Drop);
		}
	}

	return true;
}

void UValorantWeaponComponent::StartReload()
{
	if (nullptr == WeaponData || MagazineAmmo == MagazineSize || SpareAmmo <= 0)
	{
		return;
	}
	
	if (const auto* World = GetWorld())
	{
		if (false == World->GetTimerManager().IsTimerActive(ReloadHandle))
		{
			UE_LOG(LogTemp, Warning, TEXT("StartReload %p"), this);
			Character->bIsFiring = false;
			World->GetTimerManager().SetTimer(ReloadHandle, this, &UValorantWeaponComponent::Reload, 3, false, WeaponData->ReloadTime);
		}
	}
}

void UValorantWeaponComponent::Reload()
{
	const int Req = MagazineSize - MagazineAmmo;
	const int D = FMath::Min(Req, SpareAmmo);
	UE_LOG(LogTemp, Warning, TEXT("Reload Completed, MagazineAmmo : %d -> %d, SpareAmmo : %d -> %d"), MagazineAmmo, MagazineAmmo + D, SpareAmmo, SpareAmmo - D);
	MagazineAmmo += D;
	SpareAmmo -= D;
	if (const auto* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(AutoFireHandle))
		{
			Character->bIsFiring = true;
			RecoilLevel = 0;
		}
	}
}

void UValorantWeaponComponent::StopReload()
{
	if (const auto* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadHandle);
	}
}

void UValorantWeaponComponent::Drop()
{
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<UValorantWeaponComponent>())
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			UE_LOG(LogTemp, Warning, TEXT("RemoveMappingContext"));
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}

	TArray<USceneComponent*> ChildrenArray;
	GetChildrenComponents(true, ChildrenArray);
	for (auto* Component : ChildrenArray)
	{
		if (auto* PickUpComponent = Cast<UValorantPickUpComponent>(Component))
		{
			PickUpComponent->SetEnableBeginOverlap();
			break;
		}
	}
	
	DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	const FVector ForwardVector = Character->GetActorForwardVector();
	SetWorldLocation(GetComponentLocation() + ForwardVector * 300);

	OnDropWeapon.Broadcast(Character);
	Character = nullptr;
}
