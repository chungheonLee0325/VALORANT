#include "FlashComponent.h"
#include "Engine/World.h"

UFlashComponent::UFlashComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFlashComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFlashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsFlashed)
	{
		UpdateFlashEffect();
	}
}

void UFlashComponent::StartFlashEffect(float Intensity, float Duration)
{
	TargetFlashIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	FlashDuration = Duration;
	ElapsedTime = 0.0f;
	bIsFlashed = true;

	// 즉시 최대 강도로 설정
	CurrentFlashIntensity = TargetFlashIntensity;

	// 틱 활성화
	SetComponentTickEnabled(true);

	// 델리게이트 호출
	OnFlashIntensityChanged.Broadcast(CurrentFlashIntensity);
}

void UFlashComponent::StopFlashEffect()
{
	bIsFlashed = false;
	CurrentFlashIntensity = 0.0f;
	TargetFlashIntensity = 0.0f;
	ElapsedTime = 0.0f;

	// 틱 비활성화
	SetComponentTickEnabled(false);

	// 델리게이트 호출
	OnFlashIntensityChanged.Broadcast(CurrentFlashIntensity);
}

void UFlashComponent::UpdateFlashEffect()
{
	if (!bIsFlashed)
		return;

	ElapsedTime += GetWorld()->GetDeltaSeconds();

	// 시간 비율 (0.0 ~ 1.0)
	float TimeRatio = FMath::Clamp(ElapsedTime / FlashDuration, 0.0f, 1.0f);

	// 지수 감소 함수로 빠른 회복
	float RecoveryFactor = FMath::Pow(1.0f - TimeRatio, FlashRecoveryExponent);
	CurrentFlashIntensity = TargetFlashIntensity * RecoveryFactor;

	// 델리게이트 호출
	OnFlashIntensityChanged.Broadcast(CurrentFlashIntensity);

	// 섬광 효과 종료 체크
	if (TimeRatio >= 1.0f || CurrentFlashIntensity <= 0.01f)
	{
		StopFlashEffect();
	}
}
