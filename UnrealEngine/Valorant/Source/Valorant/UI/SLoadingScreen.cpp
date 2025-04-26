#include "SLoadingScreen.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateDynamicImageBrush.h"

void SLoadingScreen::SetBackgroundBrush(const FSlateBrush* InBrush)
{
	BackgroundBrush = InBrush;
}

void SLoadingScreen::Construct(const FArguments& InArgs)
{
	BackgroundBrush = InArgs._BackgroundBrush;
 
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(BackgroundBrush)
		]
	];
}

int32 SLoadingScreen::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

	FPaintGeometry BackgroundPaintGeometry = AllottedGeometry.ToPaintGeometry(
		BackgroundBrush->ImageSize,
		FSlateLayoutTransform(Center - (BackgroundBrush->ImageSize * 0.5f))
	);

	// 배경 먼저
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		BackgroundPaintGeometry,
		BackgroundBrush,
		ESlateDrawEffect::None,
		InWidgetStyle.GetColorAndOpacityTint() * BackgroundBrush->GetTint(InWidgetStyle)
	);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}