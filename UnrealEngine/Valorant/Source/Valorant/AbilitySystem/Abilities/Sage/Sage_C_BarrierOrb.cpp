#include "Sage_C_BarrierOrb.h"

#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Player/Agent/BaseAgent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "AgentAbility/Sage/BarrierOrbActor.h"
#include "AgentAbility/Sage/BarrierWallActor.h"

USage_C_BarrierOrb::USage_C_BarrierOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	
	m_AbilityID = 1001;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	FollowUpTime = 30.0f;
}

void USage_C_BarrierOrb::PrepareAbility()
{
	Super::PrepareAbility();

	// 장벽 오브 생성
	SpawnBarrierOrb();
}

void USage_C_BarrierOrb::WaitAbility()
{
	// 로컬 플레이어만 미리보기 장벽 생성
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent && OwnerAgent->IsLocallyControlled())
	{
		CreatePreviewWall();
		
		// 미리보기 업데이트 타이머 시작
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(PreviewUpdateTimer, this, 
				&USage_C_BarrierOrb::UpdateBarrierPreview, 0.01f, true);
		}
	}
}

bool USage_C_BarrierOrb::OnLeftClickInput()
{
	DestroyPreviewWall();
	// 장벽 설치
	FVector PlaceLocation = GetBarrierPlaceLocation();
	FRotator PlaceRotation = FRotator(0.f, CurrentRotation, 0.f);

	// 실제 장벽 스폰 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo))
	{
		SpawnBarrierWall(PlaceLocation, PlaceRotation);
	}
	
	// 로컬에서 이펙트 및 사운드 재생
	if (PlaceEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PlaceEffect, PlaceLocation);
	}
	
	if (PlaceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PlaceSound, PlaceLocation);
	}
	
	return true;  // 어빌리티 종료
}

bool USage_C_BarrierOrb::OnRightClickInput()
{
	// 90도 회전
	RotateBarrier();
	return false;  // 어빌리티를 종료하지 않음
}

void USage_C_BarrierOrb::SpawnBarrierOrb()
{
	if (!BarrierOrbClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 서버에서 3인칭 오브 생성 (모든 클라이언트에 자동 복제)
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("R_Hand"));
		FRotator HandRotation = OwnerAgent->GetControlRotation();
		
		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedBarrierOrb = GetWorld()->SpawnActor<ABarrierOrbActor>(
			BarrierOrbClass, HandLocation3P, HandRotation, SpawnParams3P);
		
		if (SpawnedBarrierOrb)
		{
			// 3인칭 오브로 설정 (복제되도록)
			SpawnedBarrierOrb->SetOrbViewType(EOrbViewType::ThirdPerson);
			
			// 3인칭 메쉬에 부착
			SpawnedBarrierOrb->AttachToComponent(OwnerAgent->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
			
			// 항상 설치 가능 상태로 설정
			SpawnedBarrierOrb->SetPlacementValid(true);
		}
	}
	
	// 로컬 플레이어인 경우에만 1인칭 오브 생성
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("R_Hand"));
		FRotator HandRotation = OwnerAgent->GetControlRotation();
		
		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedBarrierOrb1P = GetWorld()->SpawnActor<ABarrierOrbActor>(
			BarrierOrbClass, HandLocation1P, HandRotation, SpawnParams1P);
		
		if (SpawnedBarrierOrb1P)
		{
			// 1인칭 오브로 설정 (복제 안 됨)
			SpawnedBarrierOrb1P->SetOrbViewType(EOrbViewType::FirstPerson);
			
			// 복제 비활성화
			SpawnedBarrierOrb1P->SetReplicates(false);
			
			// 1인칭 메쉬에 부착
			SpawnedBarrierOrb1P->AttachToComponent(OwnerAgent->GetMesh1P(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
			
			// 항상 설치 가능 상태로 설정
			SpawnedBarrierOrb1P->SetPlacementValid(true);
		}
	}
}

void USage_C_BarrierOrb::DestroyBarrierOrb()
{
	// 3인칭 오브 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedBarrierOrb)
	{
		SpawnedBarrierOrb->Destroy();
		SpawnedBarrierOrb = nullptr;
	}
	
	// 1인칭 오브 제거 (로컬에서만)
	if (SpawnedBarrierOrb1P)
	{
		SpawnedBarrierOrb1P->Destroy();
		SpawnedBarrierOrb1P = nullptr;
	}
}

void USage_C_BarrierOrb::UpdateBarrierPreview()
{
	// 로컬 플레이어만 미리보기 업데이트
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent || !OwnerAgent->IsLocallyControlled())
		return;
		
	FVector PlaceLocation = GetBarrierPlaceLocation();
	FRotator PlaceRotation = FRotator(0.f, CurrentRotation, 0.f);
	
	// 미리보기 장벽 위치 업데이트
	if (PreviewBarrierWall)
	{
		PreviewBarrierWall->SetActorLocationAndRotation(PlaceLocation, PlaceRotation);
	}
}

void USage_C_BarrierOrb::RotateBarrier()
{
	// 30도씩 회전 
	CurrentRotation += RotationStep;
	
	// 0-360도 범위로 정규화
	CurrentRotation = FMath::Fmod(CurrentRotation, 360.f);
	
	// 회전 사운드 재생 (로컬에서만)
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent && OwnerAgent->IsLocallyControlled())
	{
		if (RotateSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), RotateSound);
		}
		
		// 즉시 미리보기 업데이트
		UpdateBarrierPreview();
	}
}

FVector USage_C_BarrierOrb::GetBarrierPlaceLocation()
{
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return FVector::ZeroVector;
	
	// 카메라에서 레이캐스트
	FVector CameraLocation = OwnerAgent->Camera->GetComponentLocation();
	FVector CameraForward = OwnerAgent->Camera->GetForwardVector();
	
	FHitResult HitResult;
	FVector TraceEnd = CameraLocation + CameraForward * MaxPlaceDistance;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerAgent);
	if (PreviewBarrierWall)
	{
		QueryParams.AddIgnoredActor(PreviewBarrierWall);
	}
	
	// 첫 번째 히트 지점 찾기
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		TraceEnd,
		ECC_WorldStatic,
		QueryParams
	);
	
	FVector TargetLocation;
	if (bHit)
	{
		TargetLocation = HitResult.Location;
	}
	else
	{
		TargetLocation = TraceEnd;
	}
	
	// 지면 찾기 - 더 정확한 스냅을 위해 여러 지점에서 확인
	FVector FinalLocation = TargetLocation;
	float LowestZ = TargetLocation.Z;
	bool bFoundGround = false;
	
	// 중앙과 사방향에서 지면 체크
	TArray<FVector> CheckOffsets = {
		FVector::ZeroVector,  // 중앙
		FVector(200, 0, 0),   // 전방
		FVector(-200, 0, 0),  // 후방
		FVector(0, 200, 0),   // 우측
		FVector(0, -200, 0)   // 좌측
	};
	
	for (const FVector& Offset : CheckOffsets)
	{
		FVector CheckLocation = TargetLocation + Offset;
		FHitResult GroundHit;
		FVector GroundTraceStart = CheckLocation + FVector(0, 0, 500.f);
		FVector GroundTraceEnd = CheckLocation - FVector(0, 0, 1000.f);
		
		if (GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			GroundTraceStart,
			GroundTraceEnd,
			ECC_WorldStatic,
			QueryParams))
		{
			if (!bFoundGround || GroundHit.Location.Z < LowestZ)
			{
				LowestZ = GroundHit.Location.Z;
				bFoundGround = true;
			}
		}
	}
	
	// 가장 낮은 지면에 배치
	if (bFoundGround)
	{
		FinalLocation.Z = LowestZ;
	}
	
	// 약간 지면 위로 띄우기 (겹침 방지)
	FinalLocation.Z += 15.f;
	
	return FinalLocation;
}

void USage_C_BarrierOrb::SpawnBarrierWall(FVector Location, FRotator Rotation)
{
	if (!HasAuthority(&CurrentActivationInfo) || !BarrierWallClass)
		return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = CachedActorInfo.AvatarActor.Get();
	SpawnParams.Instigator = Cast<APawn>(CachedActorInfo.AvatarActor.Get());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ABarrierWallActor* BarrierWall = GetWorld()->SpawnActor<ABarrierWallActor>(
		BarrierWallClass, Location, Rotation, SpawnParams);
	
	if (BarrierWall)
	{
		// 장벽 설정 초기화
		BarrierWall->InitializeBarrier(BarrierHealth, BarrierLifespan, BarrierBuildTime);
		
		// 건설 사운드
		if (BuildSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), BuildSound, Location);
		}
	}
}

void USage_C_BarrierOrb::CreatePreviewWall()
{
	if (!GetWorld() || !BarrierWallClass)
		return;
	
	// 로컬 플레이어만 미리보기 생성
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent || !OwnerAgent->IsLocallyControlled())
		return;
	
	// BarrierWallActor를 미리보기 모드로 생성 (로컬에서만)
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerAgent;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	PreviewBarrierWall = GetWorld()->SpawnActor<ABarrierWallActor>(
		BarrierWallClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (PreviewBarrierWall)
	{
		// 복제 비활성화 (로컬에서만 보이도록)
		PreviewBarrierWall->SetReplicates(false);
		
		// 미리보기 모드 설정
		PreviewBarrierWall->SetPreviewMode(true);
		
		// 항상 유효한 상태로 표시
		PreviewBarrierWall->SetPlacementValid(true);
		
		// 충돌 비활성화
		PreviewBarrierWall->SetActorEnableCollision(false);
		
		// Owner만 볼 수 있도록 설정
		PreviewBarrierWall->SetOnlyOwnerSee(true);
	}
}

void USage_C_BarrierOrb::DestroyPreviewWall()
{
	// 로컬에서만 미리보기 정리
	if (PreviewBarrierWall)
	{
		PreviewBarrierWall->Destroy();
		PreviewBarrierWall = nullptr;
	}
}

void USage_C_BarrierOrb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                                    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 타이머 정리 (로컬에서만)
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent && OwnerAgent->IsLocallyControlled())
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(PreviewUpdateTimer);
		}
		
		// 미리보기 정리
		DestroyPreviewWall();
	}
	
	// 오브 정리
	DestroyBarrierOrb();
	
	// 상태 초기화
	CurrentRotation = 0.f;
	
	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}