// Fill out your copyright notice in the Description page of Project Settings.

#include "KayoSuppressionZone.h"
#include "Player/Agent/BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"

AKayoSuppressionZone::AKayoSuppressionZone()
{
	// 억제 영역 크기 설정
	const float Scale = SuppressionRadius * 2.f / 100.f;
	GroundMesh->SetRelativeScale3D(FVector(Scale, Scale, 0.5f)); // 높이를 더 높게
}

void AKayoSuppressionZone::BeginPlay()
{
	Super::BeginPlay();

	// 경고(대기) 이펙트 재생 (모든 클라)
	MulticastPlayActivationEffects(GetActorLocation());

	// 1초 후 억제 효과 적용
	GetWorld()->GetTimerManager().SetTimer(SuppressionDelayHandle, this, &AKayoSuppressionZone::ApplySuppressionToAllAgents, 1.0f, false);

	// 억제 영역 시각 효과 등
	UpdateRangeIndicator();

	UE_LOG(LogTemp, Warning, TEXT("KAYO Suppression Zone activated at %s (delay before suppression)"), *GetActorLocation().ToString());
}

void AKayoSuppressionZone::ApplySuppressionToAllAgents()
{
	// 범위 내 Agent 탐색
	TArray<AActor*> FoundAgents;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), FoundAgents);

	for (AActor* Actor : FoundAgents)
	{
		ABaseAgent* Agent = Cast<ABaseAgent>(Actor);
		if (!Agent) continue;

		// 거리 체크
		if (FVector::Dist(Agent->GetActorLocation(), GetActorLocation()) <= SuppressionRadius)
		{
			ApplySuppression(Agent);
		}
	}

	// 억제 이펙트/사운드 재생 (중앙, 모든 클라)
	MulticastPlayPulseEffects(GetActorLocation());

	UE_LOG(LogTemp, Warning, TEXT("KAYO Suppression - All agents in range suppressed!"));
}

void AKayoSuppressionZone::ApplySuppression(ABaseAgent* Agent)
{
	if (!Agent || !SuppressionEffect)
	{
		return;
	}

	// 제압 효과 부여(새로운 스킬 사용불가)
	Agent->ServerApplyGE(SuppressionEffect, Cast<ABaseAgent>(Owner));
	// 시전 중인 스킬 취소
	Agent->CancelActiveAbilities();

	// 억제 적용 효과 재생 (모든 클라)
	MulticastPlaySuppressionAppliedEffects(Agent);

	UE_LOG(LogTemp, Warning, TEXT("KAYO Suppression - Applied suppression to %s"), *Agent->GetName());
}

void AKayoSuppressionZone::MulticastPlayActivationEffects_Implementation(const FVector& Location)
{
	if (ActivationEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ActivationEffect, Location);
	}
	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ActivationSound, Location);
	}
}

void AKayoSuppressionZone::MulticastPlayPulseEffects_Implementation(const FVector& Location)
{
	if (PulseEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PulseEffect, Location);
	}
	if (PulseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PulseSound, Location);
	}
}

void AKayoSuppressionZone::MulticastPlaySuppressionAppliedEffects_Implementation(ABaseAgent* Agent)
{
	if (!Agent) return;
	if (SuppressionAppliedEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(SuppressionAppliedEffect, Agent->GetRootComponent(),
													 NAME_None,
													 FVector::ZeroVector,
													 FRotator::ZeroRotator,
													 EAttachLocation::KeepRelativeOffset,
													 true,
													 true, // Auto Destroy
													 ENCPoolMethod::AutoRelease,
													 true // Auto Activate
		);
	}
	if (SuppressionAppliedSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SuppressionAppliedSound,
											  Agent->GetActorLocation());
	}
}

void AKayoSuppressionZone::UpdateRangeIndicator()
{
	if (!DynamicRangeMaterial)
	{
		return;
	}

	// 남은 시간 계산
	float RemainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(DurationTimerHandle);
	float TimeRatio = RemainingTime / SuppressionDuration;

	// 머티리얼 파라미터 업데이트
	DynamicRangeMaterial->SetScalarParameterValue(TEXT("TimeRemaining"), TimeRatio);
	DynamicRangeMaterial->SetScalarParameterValue(TEXT("PulseSpeed"), 2.0f);
	DynamicRangeMaterial->SetVectorParameterValue(TEXT("SuppressionColor"), FLinearColor(0.5f, 0.0f, 1.0f, 0.7f));
	// 보라색

#if ENABLE_DRAW_DEBUG
	// 디버그 표시
	// if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
	// {
	// 	DrawDebugSphere(GetWorld(), GetActorLocation(), SuppressionRadius, 32, FColor::Purple, false, 1.0f);
	//
	// 	FString DebugText = FString::Printf(TEXT("Suppression Zone - %.1f sec remaining"), RemainingTime);
	// 	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0, 0, 100), DebugText, nullptr, FColor::Purple,
	// 	                2.0f);
	// }
#endif
}
