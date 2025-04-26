#pragma once

#include "Widgets/SCompoundWidget.h"

class SLoadingScreen : public SCompoundWidget
{
private:
	const FSlateBrush* BackgroundBrush;

public:
	SLATE_BEGIN_ARGS(SLoadingScreen)
			: _RotatingBrush()
			, _BackgroundBrush()
			, _RotationSpeed(0.2f)
	{
	}
		
	SLATE_ARGUMENT(const FSlateBrush*, RotatingBrush)
	SLATE_ARGUMENT(const FSlateBrush*, BackgroundBrush)
	SLATE_ARGUMENT(float, RotationSpeed)
SLATE_END_ARGS()
	
	void SetBackgroundBrush(const FSlateBrush* InBrush);
	
	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};