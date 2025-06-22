#include "Sage_E_HealingOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Player/Agent/BaseAgent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "DrawDebugHelpers.h"
#include "AgentAbility/Sage/HealingOrbActor.h"

USage_E_HealingOrb::USage_E_HealingOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 1003;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	FollowUpTime = 15.0f; // 15초 동안 대기
}

void USage_E_HealingOrb::PrepareAbility()
{
	Super::PrepareAbility();
	
	// 힐링 오브 생성
	SpawnHealingOrb();
}

void USage_E_HealingOrb::WaitAbility()
{
	// 오브 위치 업데이트 타이머 시작 ToDo : 오류 해결
	// if (HasAuthority(&CurrentActivationInfo) && GetWorld())
	// {
	// 	GetWorld()->GetTimerManager().SetTimer(OrbUpdateTimer, this,
	// 	                                       &USage_E_HealingOrb::UpdateHealingOrbPosition, 0.01f, true);
	// }
}

bool USage_E_HealingOrb::OnLeftClickInput()
{
	// 아군 힐링
	//if (HasAuthority(&CurrentActivationInfo))
	{
		ABaseAgent* Target = FindHealableAlly();
		if (Target)
		{
			ApplyHealing(Target, false);
			PlayHealingEffects(Target);
			return true;
		}
	}

	// 힐링 가능한 대상이 없으면 취소
	return false;
}

bool USage_E_HealingOrb::OnRightClickInput()
{
	// 자가 힐링
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent && !OwnerAgent->IsFullHealth())
	{
		ApplyHealing(OwnerAgent, true);
		PlayHealingEffects(OwnerAgent);
		return true;
	}

	// 체력이 가득 차있으면 취소
	return false;
}

void USage_E_HealingOrb::SpawnHealingOrb()
{
	if (!HealingOrbActorClass)
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

		SpawnedHealingOrb = GetWorld()->SpawnActor<AHealingOrbActor>(
			HealingOrbActorClass, HandLocation3P, HandRotation, SpawnParams3P);

		if (SpawnedHealingOrb)
		{
			// 3인칭 오브로 설정 (복제되도록)
			SpawnedHealingOrb->SetOrbViewType(EOrbViewType::ThirdPerson);

			// 3인칭 메쉬에 부착
			SpawnedHealingOrb->AttachToComponent(OwnerAgent->GetMesh(),
			                                     FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                     FName("R_WeaponPoint"));
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

		SpawnedHealingOrb1P = GetWorld()->SpawnActor<AHealingOrbActor>(
			HealingOrbActorClass, HandLocation1P, HandRotation, SpawnParams1P);

		if (SpawnedHealingOrb1P)
		{
			// 1인칭 오브로 설정 (복제 안 됨)
			SpawnedHealingOrb1P->SetOrbViewType(EOrbViewType::FirstPerson);

			// 복제 비활성화
			SpawnedHealingOrb1P->SetReplicates(false);

			// 1인칭 메쉬에 부착
			SpawnedHealingOrb1P->AttachToComponent(OwnerAgent->GetMesh1P(),
			                                       FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                       FName("R_WeaponPoint"));
		}
	}
}

void USage_E_HealingOrb::DestroyHealingOrb()
{
	// 3인칭 오브 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedHealingOrb)
	{
		SpawnedHealingOrb->Destroy();
		SpawnedHealingOrb = nullptr;
	}

	// 1인칭 오브 제거 (로컬에서만)
	if (SpawnedHealingOrb1P)
	{
		SpawnedHealingOrb1P->Destroy();
		SpawnedHealingOrb1P = nullptr;
	}

	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(OrbUpdateTimer);
	}
}

ABaseAgent* USage_E_HealingOrb::FindHealableAlly()
{
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return nullptr;

	// 카메라 위치에서 시작
	FVector CameraLocation = OwnerAgent->Camera->GetComponentLocation();
	FVector CameraForward = OwnerAgent->Camera->GetForwardVector();

	// 구체 트레이스로 더 넓은 범위 탐지
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerAgent);
	QueryParams.bTraceComplex = false;

	// 구체 트레이스 설정
	float SphereRadius = 50.0f; // 조준 보정을 위한 구체 반경
	FVector TraceEnd = CameraLocation + CameraForward * MaxHealRange;

	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		CameraLocation,
		TraceEnd,
		FQuat::Identity,
		ECC_GameTraceChannel2, // HitDetect 채널
		FCollisionShape::MakeSphere(SphereRadius),
		QueryParams
	);

	// 디버그 표시
#if ENABLE_DRAW_DEBUG
	// DrawDebugLine(GetWorld(), CameraLocation, TraceEnd, FColor::Green, false, 0.1f, 0, 0.5f);
	// DrawDebugSphere(GetWorld(), TraceEnd, SphereRadius, 12, FColor::Green, false, 0.1f);
#endif

	// 가장 가까운 힐링 가능한 아군 찾기
	ABaseAgent* BestTarget = nullptr;
	float BestDistance = MaxHealRange;

	for (const FHitResult& Hit : HitResults)
	{
		ABaseAgent* Target = Cast<ABaseAgent>(Hit.GetActor());
		if (!Target || !IsAllyHealable(Target))
			continue;

		// 거리 확인
		float Distance = FVector::Dist(CameraLocation, Target->GetActorLocation());
		if (Distance > MaxHealRange)
			continue;

		// 각도 확인
		FVector ToTarget = (Target->GetActorLocation() - CameraLocation).GetSafeNormal();
		float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(CameraForward, ToTarget)));

		if (Angle <= MaxHealAngle && Distance < BestDistance)
		{
			BestTarget = Target;
			BestDistance = Distance;
		}
	}

	// 정확한 시야 확인
	if (BestTarget)
	{
		FHitResult LineHit;
		FVector TargetCenter = BestTarget->GetActorLocation() + FVector(0, 0, 50); // 중심 높이 조정

		bool bLineHit = GetWorld()->LineTraceSingleByChannel(
			LineHit,
			CameraLocation,
			TargetCenter,
			ECC_WorldStatic,
			QueryParams
		);

		// 시야가 가려지지 않았으면 대상 확정
		if (!bLineHit || LineHit.GetActor() == BestTarget)
		{
			CurrentHealTarget = BestTarget;
			return BestTarget;
		}
	}

	return nullptr;
}

bool USage_E_HealingOrb::IsAllyHealable(ABaseAgent* Target)
{
	if (!Target)
		return false;

	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return false;

	// 같은 팀인지 확인
	if (Target->IsAttacker() != OwnerAgent->IsAttacker())
		return false;

	// 살아있는지 확인
	if (Target->IsDead())
		return false;

	// 체력이 가득 차있지 않은지 확인
	if (Target->IsFullHealth())
		return false;

	return true;
}

void USage_E_HealingOrb::ApplyHealing(ABaseAgent* Target, bool bIsSelfHeal)
{
	if (!Target || !HasAuthority(&CurrentActivationInfo))
		return;

	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());

	// 적절한 GameplayEffect 적용
	TSubclassOf<UGameplayEffect> EffectToApply = bIsSelfHeal ? SelfHealEffect : AllyHealEffect;

	if (EffectToApply)
	{
		Target->ServerApplyGE(EffectToApply, OwnerAgent);
	}

	// 힐링 시작 사운드 재생 (멀티캐스트로 모든 클라이언트에서 재생)
	if (HealingStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HealingStartSound, Target->GetActorLocation());
	}
}

void USage_E_HealingOrb::PlayHealingEffects(ABaseAgent* Target)
{
	if (!Target)
		return;

	// 서버에서만 이펙트 생성 (자동으로 복제됨)
	if (HasAuthority(&CurrentActivationInfo))
	{
		ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
		// 힐링 이펙트 생성
		if (HealingEffect)
		{
			
			// OwnerAgent->Multicast_PlayNiagaraEffectAttached(OwnerAgent, HealingEffect, 5.0f);
			// UNiagaraComponent* HealingComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			// 	HealingEffect,
			// 	Target->GetRootComponent(),
			// 	NAME_None,
			// 	FVector::ZeroVector,
			// 	FRotator::ZeroRotator,
			// 	EAttachLocation::KeepRelativeOffset,
			// 	true,
			// 	true, // Auto Destroy
			// 	ENCPoolMethod::AutoRelease,
			// 	true // Auto Activate
			// );

			// if (HealingComp)
			// {
			// 	HealingComp->SetIsReplicated(true);
			// }
		}

		// 대상에게 힐링 표시 이펙트
		if (HealingTargetEffect)
		{
			Target->Multicast_PlayNiagaraEffectAttached(Target, HealingEffect, 5.0f);
			// UNiagaraComponent* TargetComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			// 	HealingTargetEffect,
			// 	Target->GetRootComponent(),
			// 	NAME_None,
			// 	FVector(0, 0, 50),
			// 	FRotator::ZeroRotator,
			// 	EAttachLocation::KeepRelativeOffset,
			// 	true,
			// 	true, // Auto Destroy
			// 	ENCPoolMethod::AutoRelease,
			// 	true // Auto Activate
			// );
			//
			// if (TargetComp)
			// {
			// 	TargetComp->SetIsReplicated(true);
			// }
		}
	}

	// 힐링 완료 사운드 예약 (서버에서만)
	if (HealingEndSound && GetWorld() && HasAuthority(&CurrentActivationInfo))
	{
		FTimerHandle SoundTimer;
		GetWorld()->GetTimerManager().SetTimer(SoundTimer,
		                                       [this, Target]()
		                                       {
			                                       if (IsValid(Target))
			                                       {
				                                       UGameplayStatics::PlaySoundAtLocation(
					                                       GetWorld(), HealingEndSound, Target->GetActorLocation());
			                                       }
		                                       },
		                                       HealDuration, false);
	}
}

void USage_E_HealingOrb::UpdateHealingOrbPosition()
{
	// 조준 중인 아군 찾기
	ABaseAgent* PotentialTarget = FindHealableAlly();
	bool bShouldHighlight = (PotentialTarget != nullptr);

	// 1인칭 오브 업데이트 (로컬에서만)
	if (SpawnedHealingOrb1P)
	{
		// 1인칭 오브는 로컬에서 직접 업데이트 (복제 안 됨)
		SpawnedHealingOrb1P->SetTargetHighlight(bShouldHighlight);
	}

	// 3인칭 오브 업데이트 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedHealingOrb)
	{
		// 서버에서 설정하면 자동으로 클라이언트에 복제됨
		SpawnedHealingOrb->SetTargetHighlight(bShouldHighlight);
	}
}

void USage_E_HealingOrb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                    bool bWasCancelled)
{
	// 힐링 오브 정리
	DestroyHealingOrb();

	// 현재 대상 초기화
	CurrentHealTarget = nullptr;

	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
