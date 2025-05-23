#include "FlashWidget.h"
#include "Components/Image.h"

void UFlashWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	if (FlashOverlay)
	{
		// 처음에는 투명하게
		FlashOverlay->SetOpacity(0.0f);
		FlashOverlay->SetColorAndOpacity(FLinearColor::White);
	}
}

void UFlashWidget::UpdateFlashIntensity(float Intensity)
{
	CurrentFlashIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    
	if (FlashOverlay)
	{
		// 알파값으로 섬광 강도 조절
		FlashOverlay->SetOpacity(CurrentFlashIntensity);
	}
}

void UFlashWidget::StartFlashEffect(float Duration)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UFlashWidget::StopFlashEffect()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (FlashOverlay)
	{
		FlashOverlay->SetOpacity(0.0f);
	}
}