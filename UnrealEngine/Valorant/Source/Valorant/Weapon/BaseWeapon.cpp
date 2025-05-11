// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Valorant.h"
#include "Components/WidgetComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "GameManager/ValorantGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerController.h"
#include "Player/Agent/BaseAgent.h"
#include "Net/UnrealNetwork.h"
#include "UI/DetectWidget.h"

ABaseWeapon::ABaseWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();
	
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
	if (const auto* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		DetectWidget->SetName(TEXT("획득 ") + WeaponData->LocalName);
	}
	
	// 무기 사용 여부에 따른 시각적 효과 적용
	UpdateVisualState();
	auto* WeaponMeshAsset = WeaponData->WeaponMesh;
	
	if (nullptr == WeaponMeshAsset || nullptr == Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: WeaponMeshAsset Load Fail (WeaponID : %d)"), WeaponID);
		return;
	}
	
	Mesh->SetSkeletalMeshAsset(WeaponMeshAsset);
	Mesh->SetRelativeScale3D(FVector(0.34f));

	if (WeaponData->WeaponCategory == EWeaponCategory::Sidearm)
	{
		InteractorType = EInteractorType::SubWeapon;
	}
	else
	{
		InteractorType = EInteractorType::MainWeapon;
	}
	
	MagazineSize = WeaponData->MagazineSize;
	MagazineAmmo = MagazineSize;
	// TODO: 총기별 여분탄약 데이터 추가 필요
	SpareAmmo = MagazineSize * 5;
	FireInterval = 1.0f / WeaponData->FireRate;
	for (auto Element : WeaponData->GunRecoilMap)
	{
		RecoilData.Add(Element);
	}
}

void ABaseWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (OwnerAgent && OwnerAgent->GetController() && false == bIsFiring && false == FMath::IsNearlyZero(FMath::Abs(TotalRecoilOffsetPitch) + FMath::Abs(TotalRecoilOffsetYaw), 0.05f))
	{
		const float SubPitchValue = -FMath::Lerp(TotalRecoilOffsetPitch, 0.0f, 0.88f);
		TotalRecoilOffsetPitch += SubPitchValue;
		OwnerAgent->AddControllerPitchInput(SubPitchValue);

		const float SubYawValue = -FMath::Lerp(TotalRecoilOffsetYaw, 0.0f, 0.88f);
		TotalRecoilOffsetYaw += SubYawValue;
		OwnerAgent->AddControllerYawInput(SubYawValue);

		// UE_LOG(LogTemp, Warning, TEXT("Recoil Recovery TotalRecoilOffsetPitch : %f, TotalRecoilOffsetYaw : %f"), TotalRecoilOffsetPitch, TotalRecoilOffsetYaw);
	}
}

void ABaseWeapon::StartFire()
{
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetController())
	{
		return;
	}

	bIsFiring = true;
	if (FMath::IsNearlyZero(FMath::Abs(TotalRecoilOffsetPitch) + FMath::Abs(TotalRecoilOffsetYaw), 0.05f))
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
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetController() || false == bIsFiring)
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
	
	// 무기를 사용한 것으로 표시
	if (!bWasUsed && HasAuthority())
	{
		bWasUsed = true;
	}
	
	// KBD: 발사 시 캐릭터에 반동값 적용
	if (RecoilData.Num() > 0)
	{
		const float PitchValue = RecoilData[RecoilLevel].OffsetPitch;
		OwnerAgent->AddControllerPitchInput(PitchValue);
		TotalRecoilOffsetPitch += PitchValue;

		const float YawValue = RecoilData[RecoilLevel].OffsetYaw;
		OwnerAgent->AddControllerYawInput(YawValue);
		TotalRecoilOffsetYaw += YawValue;
		
		RecoilLevel = FMath::Clamp(RecoilLevel + 1, 0, RecoilData.Num() - 1);
		
		// UE_LOG(LogTemp, Warning, TEXT("Ammo : %d, Total : (%f, %f), Add : (%f, %f)"), MagazineAmmo, TotalRecoilOffsetPitch, TotalRecoilOffsetYaw, PitchValue, YawValue);
	}

	// 서버 쪽에서 처리해야 하는 발사 로직 호출
	// 서버 입장에서는 클라이언트의 ViewportSize를 모르고, 반응성 등의 문제 때문에 발사 지점, 방향은 클라에서 계산 후 넘겨줌
	const auto* PlayerController = Cast<AAgentPlayerController>(OwnerAgent->GetController());
	if (nullptr == PlayerController)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, Agent Controller is incorrect"), __FUNCTION__);
		return;
	}
	int32 ScreenWidth, ScreenHeight;
	PlayerController->GetViewportSize(ScreenWidth, ScreenHeight);
	FVector Start, Dir;
	PlayerController->DeprojectScreenPositionToWorld(ScreenWidth * 0.5f, ScreenHeight * 0.5f, Start, Dir);
	ServerRPC_Fire(Start, Dir);
	
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

void ABaseWeapon::ServerRPC_Fire_Implementation(const FVector& Location, const FVector& Direction)
{
	const auto* WorldContext = GetWorld();
	if (nullptr == WorldContext)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, World is nullptr"), __FUNCTION__);
		return;
	}
	
	const FVector& Start = Location;
	const FVector End = Start + Direction * 99999;
	// NET_LOG(LogTemp, Warning, TEXT("ServerRPC_Fire_Implementation, Start : %s, End : %s"), *Start.ToString(), *End.ToString());
	
	// 궤적, 탄착군 디버깅
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	ActorsToIgnore.Add(OwnerAgent);
	FHitResult OutHit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		WorldContext,
		Start,
		End,
		UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel2), // TraceChannel: HitDetect
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		OutHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		2.5f
	);
	if (bHit)
	{
		int FinalDamage = WeaponData->BaseDamage;
		switch (const EAgentDamagedPart DamagedPart = ABaseAgent::GetHitDamagedPart(OutHit.BoneName))
		{
		case EAgentDamagedPart::None:
			break;
		case EAgentDamagedPart::Head:
			FinalDamage *= WeaponData->HeadshotMultiplier;
			break;
		case EAgentDamagedPart::Body:
			break;
		case EAgentDamagedPart::Legs:
			FinalDamage *= WeaponData->LegshotMultiplier;
			break;
		}

		const auto& FalloffArray = WeaponData->GunDamageFalloffArray;
		for (int i = FalloffArray.Num() - 1; i >= 0; i--)
		{
			const auto& FalloffData = FalloffArray[i];
			if (OutHit.Distance >= FalloffData.RangeStart)
			{
				FinalDamage *= FalloffData.DamageMultiplier;
				break;
			}
		}
		FinalDamage = FMath::Clamp(FinalDamage, 1, 9999);
		
		NET_LOG(LogTemp, Warning, TEXT("LineTraceSingle Hit: %s, BoneName: %s, Distance: %f, FinalDamage: %d"), *OutHit.GetActor()->GetName(), *OutHit.BoneName.ToString(), OutHit.Distance, FinalDamage);
		if (ABaseAgent* HitAgent = Cast<ABaseAgent>(OutHit.GetActor()))
		{
			HitAgent->ServerApplyHitScanGE(NewDamageEffectClass, FinalDamage);
		}
		DrawDebugPoint(WorldContext, OutHit.ImpactPoint, 5, FColor::Green, false, 30);
	}
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
	
	// 무기를 사용한 것으로 표시
	if (!bWasUsed && HasAuthority())
	{
		bWasUsed = true;
	}
	
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

bool ABaseWeapon::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	// TODO: 현재 이미 똑같은 종류의 무기를 들고 있을 경우 false 반환
	return Super::ServerOnly_CanAutoPickUp(Agent);
}

bool ABaseWeapon::ServerOnly_CanDrop() const
{
	// TODO: 근접무기인 경우 false
	return Super::ServerOnly_CanDrop();
}

void ABaseWeapon::ServerRPC_PickUp(ABaseAgent* Agent)
{
	Super::ServerRPC_PickUp(Agent);
	ServerOnly_AttachWeapon(Agent);
}

void ABaseWeapon::ServerRPC_Drop()
{
	Super::ServerRPC_Drop();

	//TODO: 이미 Super에서 Onwer가 Null로 처리됨. 필요시 수정
	if (nullptr == OwnerAgent)
	{
		return;
	}
	
	if (const AAgentPlayerController* PlayerController = Cast<AAgentPlayerController>(OwnerAgent->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// UE_LOG(LogTemp, Warning, TEXT("RemoveMappingContext"));
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}

void ABaseWeapon::ServerRPC_Interact(ABaseAgent* InteractAgent)
{
	Super::ServerRPC_Interact(InteractAgent);
	if (ServerOnly_CanInteract())
	{
		ServerRPC_PickUp(InteractAgent);
	}
}

void ABaseWeapon::ServerOnly_AttachWeapon(ABaseAgent* Agent)
{
	if (nullptr == Agent)
	{
		return;
	}
	
	FAttachmentTransformRules AttachmentRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		true
	);
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
			// EnhancedInputComponent->BindAction(StartFireAction, ETriggerEvent::Triggered, this, &ABaseWeapon::StartFire);
			// EnhancedInputComponent->BindAction(EndFireAction, ETriggerEvent::Triggered, this, &ABaseWeapon::EndFire);
			// EnhancedInputComponent->BindAction(StartReloadAction, ETriggerEvent::Triggered, this, &ABaseWeapon::StartReload);
		}
	}

	Agent->AcquireInteractor(this);
}

void ABaseWeapon::SetWeaponID(int32 NewWeaponID)
{
	WeaponID = NewWeaponID;
	
	// 무기 ID가 변경되었으므로 무기 데이터 다시 로드
	auto* GameInstance = Cast<UValorantGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInstance)
	{
		WeaponData = GameInstance->GetWeaponData(WeaponID);
		if (WeaponData)
		{
			// 무기 데이터 기반으로 속성 업데이트
			MagazineSize = WeaponData->MagazineSize;
			MagazineAmmo = MagazineSize;
			// 여분 탄약 설정 (추후 데이터 추가 필요)
			SpareAmmo = MagazineSize * 5;
			FireInterval = 1.0f / WeaponData->FireRate;
			
			// 반동 데이터 갱신
			RecoilData.Empty();
			for (auto Element : WeaponData->GunRecoilMap)
			{
				RecoilData.Add(Element);
			}
			
			// 메시 업데이트 (실제 구현에서는 무기 ID에 따라 다른 메시 적용)
			if (WeaponData->WeaponMesh != nullptr)
			{
				Mesh->SetSkeletalMesh(WeaponData->WeaponMesh);
				// TODO: 여기서 무기 ABP 로드 로직 추가 OR BP 로딩으로 변경
			}
		}
	}
}

// 무기 사용 여부를 네트워크 복제되도록 처리
void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 무기 사용 여부 복제
	DOREPLIFETIME(ABaseWeapon, bWasUsed);
}

// 라운드 시작/종료 시 무기 사용 여부 리셋을 위한 함수 추가
void ABaseWeapon::ResetUsedStatus()
{
	if (HasAuthority())
	{
		bWasUsed = false;
	}
}

// 무기 사용 여부에 따른 시각적 효과 업데이트
void ABaseWeapon::UpdateVisualState()
{
	// // 사용하지 않은 무기는 약간 밝게 표시하여 구분하기 쉽게 함
	// if (WeaponMesh)
	// {
	// 	// 기본 색상 파라미터 (Material Instance를 통해 접근)
	// 	if (!bWasUsed)
	// 	{
	// 		// 미사용 무기는 약간 아웃라인 효과 또는 하이라이트
	// 		// Material Instance로 처리하는 것이 이상적이나, 여기서는 간단히 색상 변경으로 대체
	// 		// 실제 구현에서는 적절한 Material Parameter를 설정
	// 		WeaponMesh->SetRenderCustomDepth(true);  // 아웃라인을 위한 커스텀 뎁스 활성화
	// 	}
	// 	else
	// 	{
	// 		// 사용된 무기는 일반 효과
	// 		WeaponMesh->SetRenderCustomDepth(false);
	// 	}
	// }
}

// 무기 사용 여부 설정시 시각적 상태도 업데이트하도록 수정
void ABaseWeapon::SetWasUsed(bool bNewWasUsed)
{
	if (bWasUsed != bNewWasUsed)
	{
		bWasUsed = bNewWasUsed;
		UpdateVisualState();
	}
}