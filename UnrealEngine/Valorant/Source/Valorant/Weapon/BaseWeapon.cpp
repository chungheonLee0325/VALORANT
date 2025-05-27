// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"

#include "BaseWeaponAnim.h"
#include "ThirdPersonInteractor.h"
#include "Valorant.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "GameManager/ValorantGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerController.h"
#include "Player/Agent/BaseAgent.h"
#include "Net/UnrealNetwork.h"
#include "UI/DetectWidget.h"
#include "NiagaraFunctionLibrary.h"
//#include "NiagaraCommon.h"

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
		// DetectWidget->SetName(TEXT("획득 ") + WeaponData->LocalName);
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

	// NET_LOG(LogTemp, Warning, TEXT("%hs Called, WeaponId: %d"), __FUNCTION__, WeaponData->WeaponID);
	if (WeaponData->GunABPClass)
	{
		// NET_LOG(LogTemp, Warning, TEXT("%hs Called, WeaponData->GunABPClass, %s"), __FUNCTION__, *GetName());
		Mesh->SetAnimInstanceClass(WeaponData->GunABPClass);
	}

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

	AM_Fire = WeaponData->FireAnim;
	AM_Reload = WeaponData->ReloadAnim;
}

void ABaseWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (OwnerAgent && OwnerAgent->GetController() && (false == bIsFiring || MagazineAmmo <= 0))
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
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetController() || true == bIsReloading)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OwnerAgent or Controller is nullptr"), __FUNCTION__);
		return;
	}

	bIsFiring = true;
	if (FMath::IsNearlyZero(FMath::Abs(TotalRecoilOffsetPitch) + FMath::Abs(TotalRecoilOffsetYaw), 0.05f))
	{
		RecoilLevel = 0;
		TotalRecoilOffsetPitch = 0.0f;
		TotalRecoilOffsetYaw = 0.0f;
	}
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, RecoilLevel: %d"), __FUNCTION__, RecoilLevel);

	GetWorld()->GetTimerManager().ClearTimer(RecoverRecoilLevelHandle);
	// InRate가 0.01인 이유는 단순히 체크하는 용도이지 FireInterval에 따른 실제 사격은 Fire에서 체크하기 때문
	GetWorld()->GetTimerManager().SetTimer(AutoFireHandle, this, &ABaseWeapon::Fire, 0.01f, true, 0);
}

void ABaseWeapon::Fire()
{
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetController() || false == bIsFiring || true == bIsReloading)
	{
		return;
	}

	if (MagazineAmmo <= 0)
	{
		if (SpareAmmo > 0)
		{
			ServerRPC_StartReload();
		}
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime + FireInterval > CurrentTime)
	{
		return;
	}
	LastFireTime = CurrentTime;

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
		// NET_LOG(LogTemp, Warning, TEXT("Ammo : %d, Total : (%f, %f), Add : (%f, %f)"), MagazineAmmo, TotalRecoilOffsetPitch, TotalRecoilOffsetYaw, PitchValue, YawValue);
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
}

FVector ABaseWeapon::GetSpreadDirection(const FVector& Direction)
{
	float MaxAngleDeg = 0;
	// 일정 속도 이상으로 이동 또는 점프 중일 때 MaxAngleRad = 5
	if (OwnerAgent)
	{
		if (const auto* MovementComponent = OwnerAgent->GetMovementComponent())
		{
			if (MovementComponent->Velocity.Size() >= 100.f || false == MovementComponent->IsMovingOnGround())
			{
				MaxAngleDeg = 5.f;
			}
		}
	}

	// TODO: 무기 종류에 따라 탄퍼짐 계수 다르게
	// if (WeaponData->WeaponCategory == EWeaponCategory::SMG)

	const float MaxAngleRad = FMath::DegreesToRadians(MaxAngleDeg);

	const float Yaw = FMath::RandRange(-MaxAngleRad, MaxAngleRad);
	const float Pitch = FMath::RandRange(-MaxAngleRad, MaxAngleRad);
	FRotator SpreadRot = Direction.Rotation();
	SpreadRot.Yaw += FMath::RadiansToDegrees(Yaw);
	SpreadRot.Pitch += FMath::RadiansToDegrees(Pitch);
	return SpreadRot.Vector();
}

void ABaseWeapon::ServerRPC_Fire_Implementation(const FVector& Location, const FVector& Direction)
{
	const auto* WorldContext = GetWorld();
	if (nullptr == WorldContext)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, World is nullptr"), __FUNCTION__);
		return;
	}

	MagazineAmmo--;
	OnRep_Ammo();
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, MagazineAmmo: %d, SpareAmmo: %d"), __FUNCTION__, MagazineAmmo,
	        SpareAmmo);

	// 무기를 사용한 것으로 표시
	bWasUsed = true;
	
	Multicast_SpawnMuzzleFlash();
	MulticastRPC_PlayFireSound();
	MulticastRPC_PlayFireAnimation();
	
	const FVector& Dir = GetSpreadDirection(Direction);
	const FVector& Start = Location;
	const FVector End = Start + Dir * 99999;

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
		EDrawDebugTrace::None,
		OutHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		2.5f
	);
	if (bHit)
	{
		// 팀킬 방지 로직 추가
		if (ABaseAgent* HitAgent = Cast<ABaseAgent>(OutHit.GetActor()))
		{
			// 공격자와 피격자가 같은 팀인지 확인
			if (OwnerAgent && HitAgent->IsBlueTeam() == OwnerAgent->IsBlueTeam())
			{
				// 같은 팀이면 데미지를 주지 않고 임팩트 효과만 재생
				// Multicast_SpawnImpactEffect(OutHit.ImpactPoint, OutHit.ImpactNormal.Rotation());
				// Multicast_SpawnTracer(Start, OutHit.ImpactPoint);
			}
			else
			{
				int FinalDamage = WeaponData->BaseDamage;
				const EAgentDamagedPart DamagedPart = ABaseAgent::GetHitDamagedPart(OutHit.BoneName);
				switch (DamagedPart)
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

				// 피격 방향 판정
				EAgentDamagedDirection DamagedDirection = EAgentDamagedDirection::Front;
				const FVector HitDir = (OutHit.TraceStart - OutHit.TraceEnd).GetSafeNormal();
				FVector Forward = HitAgent->GetActorForwardVector();
				FVector Cross = FVector::CrossProduct(Forward, -HitDir);
				const float Dot = FVector::DotProduct(Forward, -HitDir);
				if (Dot > 0.5f) DamagedDirection = EAgentDamagedDirection::Back;
				else if (Dot < -0.5f) DamagedDirection = EAgentDamagedDirection::Front;
				else if (Cross.Z > 0) DamagedDirection = EAgentDamagedDirection::Left;
				else DamagedDirection = EAgentDamagedDirection::Right;

				NET_LOG(LogTemp, Warning, TEXT("LineTraceSingle Hit: %s, BoneName: %s, Distance: %f, FinalDamage: %d"),
						*OutHit.GetActor()->GetName(), *OutHit.BoneName.ToString(), OutHit.Distance, FinalDamage);
			
				// 공격자 정보 전달
				HitAgent->ServerApplyHitScanGE(NewDamageEffectClass, FinalDamage, OwnerAgent, DamagedPart, DamagedDirection);
			}
		}
		// DrawDebugPoint(WorldContext, OutHit.ImpactPoint, 5, FColor::Green, false, 30);

		// 벽이든 캐릭터든 항상 임팩트 이펙트/사운드 호출
		Multicast_SpawnImpactEffect(OutHit.ImpactPoint, OutHit.ImpactNormal.Rotation());
		Multicast_SpawnTracer(Start, bHit ? OutHit.ImpactPoint : End);
	}
}

void ABaseWeapon::EndFire()
{
	bIsFiring = false;

	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	GetWorld()->GetTimerManager().SetTimer(RecoverRecoilLevelHandle, this, &ABaseWeapon::RecoverRecoilLevel,
	                                       FireInterval / 2, true);
}

void ABaseWeapon::RecoverRecoilLevel()
{
	RecoilLevel = FMath::Clamp(RecoilLevel - 1, 0, RecoilData.Num() - 1);
	if (RecoilLevel == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoverRecoilLevelHandle);
	}
}

void ABaseWeapon::ServerRPC_StartReload_Implementation()
{
	if (nullptr == WeaponData || MagazineAmmo == MagazineSize || SpareAmmo <= 0)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Can't Reload"), __FUNCTION__);
		return;
	}

	if (false == GetWorld()->GetTimerManager().IsTimerActive(ReloadHandle))
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Reload Start"), __FUNCTION__);
		bIsFiring = false;
		bIsReloading = true;
		MulticastRPC_PlayReloadAnimation();
		GetWorld()->GetTimerManager().SetTimer(ReloadHandle, this, &ABaseWeapon::Reload, 3, false,
		                                       WeaponData->ReloadTime);
	}
}

void ABaseWeapon::MulticastRPC_PlayFireSound_Implementation()
{
	if (WeaponData && WeaponData->FireSound)
	{
		bool bIsFP = false;
		bool bIsCU = false;
		int Dir = 0;
		const auto* LocallyPC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>();
		if (LocallyPC && LocallyPC->IsLocalController())
		{
			if (const auto* ViewTarget = LocallyPC->GetViewTarget())
			{
				if (OwnerAgent == ViewTarget)
				{
					bIsFP = true;
				}
				else
				{
					const auto& FireLocation = GetActorLocation();
					const auto& MyLocation = ViewTarget->GetActorLocation();
					const auto& MyForward = ViewTarget->GetActorForwardVector();
					const auto ToFireLocation = (FireLocation - MyLocation).GetSafeNormal();
					// 발사 지점과의 거리가 2000보다 작으면 근접 발사 사운드 출력
					bIsCU = FVector::Dist(FireLocation, MyLocation) <= 2000.f;
					// 내 전방 벡터를 기준으로 총소리가 어디서 들린 것인지 판단
					const auto Dot = FVector::DotProduct(MyForward, ToFireLocation);
					if (Dot > 0.7f)
						Dir = 0; // 전방
					else if (Dot < -0.7f)
						Dir = 1; // 후방
					else
						Dir = 2; // 사이드
				}
			}
		}
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, bIsFP: %hs, bIsCU: %hs, Dir: %d"), __FUNCTION__, bIsFP ? "True" : "False", bIsCU ? "True" : "False", Dir);
		PlayFireSound(WeaponData->FireSound, true, true, 0, WeaponData->MuzzleSocketName);
	}
}

void ABaseWeapon::MulticastRPC_PlayFireAnimation_Implementation()
{
	if (AM_Fire == nullptr)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, AM_Fire is nullptr"), __FUNCTION__);
		return;
	}

	if (nullptr == OwnerAgent)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, OwnerAgent is nullptr"), __FUNCTION__);
		return;
	}
	
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnFire.Broadcast();
	if (OwnerAgent)
	{
		OwnerAgent->OnFire();
	}
}

void ABaseWeapon::MulticastRPC_PlayReloadAnimation_Implementation()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnReload.Broadcast();
	if (OwnerAgent)
	{
		OwnerAgent->OnReload();
	}
}



void ABaseWeapon::Reload()
{
	const int Req = MagazineSize - MagazineAmmo;
	const int D = FMath::Min(Req, SpareAmmo);
	UE_LOG(LogTemp, Warning, TEXT("Reload Completed, MagazineAmmo : %d -> %d, SpareAmmo : %d -> %d"), MagazineAmmo,
	       MagazineAmmo + D, SpareAmmo, SpareAmmo - D);
	MagazineAmmo += D;
	SpareAmmo -= D;
	OnRep_Ammo();

	// 무기를 사용한 것으로 표시
	bWasUsed = true;

	if (const auto* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(AutoFireHandle))
		{
			bIsFiring = true;
			RecoilLevel = 0;
		}
	}

	bIsReloading = false;
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
	// 요원이 현재 똑같은 종류의 무기를 들고 있을 경우 false 반환
	switch (GetWeaponCategory())
	{
	case EWeaponCategory::None:
		return false;
	case EWeaponCategory::Sidearm:
		return Agent->GetSubWeapon() == nullptr;
	case EWeaponCategory::SMG:
		return Agent->GetMainWeapon() == nullptr;
	case EWeaponCategory::Shotgun:
		return Agent->GetMainWeapon() == nullptr;
	case EWeaponCategory::Rifle:
		return Agent->GetMainWeapon() == nullptr;
	case EWeaponCategory::Sniper:
		return Agent->GetMainWeapon() == nullptr;
	case EWeaponCategory::Heavy:
		return Agent->GetMainWeapon() == nullptr;
	}
	return false;
}

bool ABaseWeapon::ServerOnly_CanDrop() const
{
	return Super::ServerOnly_CanDrop();
}

void ABaseWeapon::ServerRPC_PickUp_Implementation(ABaseAgent* Agent)
{
	Super::ServerRPC_PickUp_Implementation(Agent);
	if (Agent == nullptr)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, InteractAgent is nullptr"), __FUNCTION__);
		return;
	}
	ServerOnly_AttachWeapon(Agent);
}

void ABaseWeapon::ServerRPC_Drop_Implementation()
{
	Super::ServerRPC_Drop_Implementation();
}

void ABaseWeapon::ServerRPC_Interact_Implementation(ABaseAgent* InteractAgent)
{
	Super::ServerRPC_Interact_Implementation(InteractAgent);

	if (InteractAgent == nullptr)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, InteractAgent is nullptr"), __FUNCTION__);
		return;
	}
	if (ServerOnly_CanInteract())
	{
		ServerRPC_PickUp(InteractAgent);
	}
}

void ABaseWeapon::ServerOnly_AttachWeapon(ABaseAgent* Agent)
{
	if (nullptr == Agent)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, InteractAgent is nullptr"), __FUNCTION__);
		return;
	}

	FAttachmentTransformRules AttachmentRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		true
		);
	Mesh->AttachToComponent(Agent->GetMesh1P(), AttachmentRules, FName(TEXT("R_WeaponSocket")));
	if (nullptr != ThirdPersonInteractor)
	{
		ThirdPersonInteractor->Destroy();
		ThirdPersonInteractor = nullptr;
	}
	if ((ThirdPersonInteractor = GetWorld()->SpawnActor<AThirdPersonInteractor>()))
	{
		ThirdPersonInteractor->SetOwner(Agent);
		ThirdPersonInteractor->MulticastRPC_InitWeapon(this, WeaponID);
		ThirdPersonInteractor->AttachToComponent(Agent->GetMesh(), AttachmentRules, FName(TEXT("R_WeaponSocket")));
	}

	Agent->AcquireInteractor(this);
}

void ABaseWeapon::NetMulti_ReloadWeaponData_Implementation(int32 NewWeaponID)
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

void ABaseWeapon::OnRep_Ammo() const
{
	if (OwnerAgent && OwnerAgent->IsLocallyControlled())
	{
		if (const auto* Controller = OwnerAgent->GetController<AAgentPlayerController>())
		{
			if (WeaponData && WeaponData->WeaponCategory != EWeaponCategory::Melee)
			{
				Controller->NotifyChangedAmmo(true, MagazineAmmo, SpareAmmo);
			}
			else
			{
				Controller->NotifyChangedAmmo(false, MagazineAmmo, SpareAmmo);
			}
		}
	}
}

void ABaseWeapon::Multicast_SetActive_Implementation(bool bActive)
{
	Super::Multicast_SetActive_Implementation(bActive);
	if (bActive)
	{
		OnRep_Ammo();
	}
}

// 무기 사용 여부를 네트워크 복제되도록 처리
void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 무기 사용 여부 복제
	DOREPLIFETIME(ABaseWeapon, bWasUsed);
	DOREPLIFETIME(ABaseWeapon, MagazineAmmo);
	DOREPLIFETIME(ABaseWeapon, SpareAmmo);
	DOREPLIFETIME(ABaseWeapon, bIsReloading);
	DOREPLIFETIME(ABaseWeapon, WeaponID);
}

// 라운드 시작/종료 시 무기 사용 여부 리셋을 위한 함수 추가
void ABaseWeapon::ResetUsedStatus()
{
	if (HasAuthority())
	{
		bWasUsed = false;
	}
}

void ABaseWeapon::SetWeaponID(const int NewWeaponID)
{
	this->WeaponID = NewWeaponID;
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

void ABaseWeapon::ServerOnly_ClearAmmo()
{
	MagazineAmmo = MagazineSize;
	// TODO: 총기별 여분탄약 데이터 추가 필요
	SpareAmmo = MagazineSize * 5;
	OnRep_Ammo();
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

void ABaseWeapon::Multicast_SpawnMuzzleFlash_Implementation()
{
	if (WeaponData && WeaponData->MuzzleFlashEffect && Mesh)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			WeaponData->MuzzleFlashEffect,
			Mesh,
			WeaponData->MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::Type::SnapToTarget, true
		);
	}
}

void ABaseWeapon::Multicast_SpawnTracer_Implementation(const FVector& Start, const FVector& End)
{
	if (WeaponData && WeaponData->TracerEffect && Mesh)
	{
		// 머즐 소켓 위치에서 End를 향해 발사
		FVector MuzzleLoc = Mesh->GetSocketLocation(WeaponData->MuzzleSocketName);
		FVector Dir = End - MuzzleLoc;
		float Length = Dir.Size();
		FRotator Rot = Dir.Rotation();
		
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			WeaponData->TracerEffect,
			MuzzleLoc,
			Rot
		);
	}
}

void ABaseWeapon::Multicast_SpawnImpactEffect_Implementation(const FVector& Location, const FRotator& Rotation)
{
	if (WeaponData && WeaponData->ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			WeaponData->ImpactEffect,
			Location,
			Rotation
		);
	}
}
