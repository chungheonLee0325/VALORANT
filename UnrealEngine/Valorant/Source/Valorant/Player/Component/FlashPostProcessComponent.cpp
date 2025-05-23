#include "FlashPostProcessComponent.h"
#include "Player/Agent/BaseAgent.h"

UFlashPostProcessComponent::UFlashPostProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlashPostProcessComponent::BeginPlay()
{
	Super::BeginPlay();
    
	// 소유자의 카메라 찾기
	if (ABaseAgent* Agent = Cast<ABaseAgent>(GetOwner()))
	{
		OwnerCamera = Agent->Camera;
		if (OwnerCamera && !bOriginalSettingsSaved)
		{
			OriginalSettings = OwnerCamera->PostProcessSettings;
			bOriginalSettingsSaved = true;
		}
	}
}

void UFlashPostProcessComponent::UpdateFlashPostProcess(float FlashIntensity)
{
	if (!OwnerCamera || !bOriginalSettingsSaved)
		return;

	if (FlashIntensity <= 0.01f)
	{
		// 섬광이 거의 없으면 원본 설정으로 복원
		OwnerCamera->PostProcessSettings = OriginalSettings;
		OwnerCamera->PostProcessBlendWeight = 0.0f;
		return;
	}

	// 포스트 프로세스 설정 업데이트
	OwnerCamera->PostProcessSettings.bOverride_ColorGamma = true;
	OwnerCamera->PostProcessSettings.bOverride_ColorSaturation = true;
    
	float GammaMultiplier = 1.0f + (FlashIntensity * (MaxBrightness - 1.0f));
	OwnerCamera->PostProcessSettings.ColorGamma = FVector4(GammaMultiplier, GammaMultiplier, GammaMultiplier, 1.0f);
    
	float SaturationValue = 1.0f - (FlashIntensity * (1.0f - MaxSaturation));
	OwnerCamera->PostProcessSettings.ColorSaturation = FVector4(SaturationValue, SaturationValue, SaturationValue, 1.0f);
    
	OwnerCamera->PostProcessBlendWeight = FlashIntensity;
}