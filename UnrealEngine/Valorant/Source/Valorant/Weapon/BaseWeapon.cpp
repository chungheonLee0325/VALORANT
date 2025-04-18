// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "PickUpComponent.h"
#include "GameManager/ValorantGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerController.h"
#include "Player/Agent/BaseAgent.h"

ABaseWeapon::ABaseWeapon()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	PickUpModule = CreateDefaultSubobject<UPickUpComponent>(TEXT("PickUpModule"));
	PickUpModule->SetupAttachment(GetRootComponent());
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	PickUpModule->OnPickUp.AddDynamic(this, &ABaseWeapon::AttachWeapon);
	
	auto* GameInstance = Cast<UValorantGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (nullptr == GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: GameInstance Is Null"));
		return;
	}

	WeaponData = GameInstance->GetWeaponData(WeaponID);
	if (nullptr == WeaponData)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: WeaponData Load Fail (WeaponID : %d)"), WeaponID);
		return;
	}

	// TODO: WeaponID에 맞는 SkeletalMesh 불러오기
	FStringAssetReference MeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/FPWeapon/Mesh/SK_FPGun.SK_FPGun'"));
	auto* WeaponMeshAsset = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *MeshRef.ToString()));
	if (nullptr == WeaponMeshAsset || nullptr == WeaponMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: WeaponMeshAsset Load Fail (WeaponID : %d)"), WeaponID);
		return;
	}
	WeaponMesh->SetSkeletalMeshAsset(WeaponMeshAsset);
	
	MagazineSize = WeaponData->MagazineSize;
	MagazineAmmo = MagazineSize;
	// TODO: 총기별 여분탄약 데이터 추가 필요
	SpareAmmo = MagazineSize * 3;
	FireInterval = 1.0f / WeaponData->FireRate;
	for (auto Element : WeaponData->GunRecoilMap)
	{
		RecoilData.Add(Element);
	}
}

void ABaseWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ABaseWeapon::AttachWeapon(ABaseAgent* PickUpAgent)
{
	Agent = PickUpAgent;

	if (nullptr == Agent)
	{
		return;
	}

	// if (const auto* Owner = GetOwner())
	// {
	// 	if (auto* PickUpUI = Owner->FindComponentByClass<UWidgetComponent>())
	// 	{
	// 		PickUpUI->SetVisibility(false);
	// 	}
	// }

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Agent->GetMesh(), AttachmentRules, FName(TEXT("R_WeaponPointSocket")));

	// Set up action bindings
	if (const AAgentPlayerController* PlayerController = Cast<AAgentPlayerController>(Agent->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(StartFireAction, ETriggerEvent::Triggered, this, &ABaseWeapon::StartFire);
			EnhancedInputComponent->BindAction(EndFireAction, ETriggerEvent::Triggered, this, &ABaseWeapon::EndFire);
			EnhancedInputComponent->BindAction(StartReloadAction, ETriggerEvent::Triggered, this, &ABaseWeapon::StartReload);
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Triggered, this, &ABaseWeapon::Drop);
		}
	}
}

void ABaseWeapon::StartFire()
{
	if (nullptr == Agent || nullptr == Agent->GetController())
	{
		return;
	}

	bIsFiring = true;
	if (FMath::IsNearlyZero(TotalRecoilOffsetPitch + TotalRecoilOffsetYaw, 0.1f))
	{
		RecoilLevel = 0;
		TotalRecoilOffsetPitch = 0.0f;
		TotalRecoilOffsetYaw = 0.0f;
	}
	UE_LOG(LogTemp, Warning, TEXT("StartFire, RecoilLevel : %d"), RecoilLevel);

	GetWorld()->GetTimerManager().SetTimer(AutoFireHandle, this, &ABaseWeapon::Fire, 0.01f, true, 0);
}

void ABaseWeapon::Fire()
{
	if (nullptr == Agent || nullptr == Agent->GetController() || false == bIsFiring)
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
		Agent->AddControllerPitchInput(PitchValue);
		TotalRecoilOffsetPitch += PitchValue;

		const float YawValue = RecoilData[RecoilLevel].OffsetYaw;
		Agent->AddControllerYawInput(YawValue);
		TotalRecoilOffsetYaw += YawValue;
		
		RecoilLevel = FMath::Clamp(RecoilLevel + 1, 0, RecoilData.Num() - 1);
		
		UE_LOG(LogTemp, Warning, TEXT("Ammo : %d, Total : (%f, %f), Add : (%f, %f)"), MagazineAmmo, TotalRecoilOffsetPitch, TotalRecoilOffsetYaw, PitchValue, YawValue);
	}

	if (const UWorld* World = GetWorld())
	{
		const AAgentPlayerController* PlayerController = Cast<AAgentPlayerController>(Agent->GetController());
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
	
	// // Try and play the sound if specified
	// if (FireSound != nullptr)
	// {
	// 	UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	// }
	//
	// // Try and play a firing animation if specified
	// if (FireAnimation != nullptr)
	// {
	// 	// Get the animation object for the arms mesh
	// 	UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
	// 	if (AnimInstance != nullptr)
	// 	{
	// 		AnimInstance->Montage_Play(FireAnimation, 1.f);
	// 	}
	// }
}

void ABaseWeapon::EndFire()
{
	bIsFiring = false;

	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
}

void ABaseWeapon::StartReload()
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
			bIsFiring = false;
			World->GetTimerManager().SetTimer(ReloadHandle, this, &ABaseWeapon::Reload, 3, false, WeaponData->ReloadTime);
		}
	}
}

void ABaseWeapon::Reload()
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
			bIsFiring = true;
			RecoilLevel = 0;
		}
	}
}

void ABaseWeapon::StopReload()
{
	if (const auto* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadHandle);
	}
}

void ABaseWeapon::Drop()
{
	if (nullptr == Agent)
	{
		return;
	}

	if (const AAgentPlayerController* PlayerController = Cast<AAgentPlayerController>(Agent->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// UE_LOG(LogTemp, Warning, TEXT("RemoveMappingContext"));
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}

	PickUpModule->SetEnableBeginOverlap();

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	const FVector ForwardVector = Agent->GetActorForwardVector();
	SetActorLocation(GetActorLocation() + ForwardVector * 300);
	
	Agent = nullptr;
}