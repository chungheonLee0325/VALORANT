#include "LoadingScreen.h"

#include "SLoadingScreen.h"

#define LOCTEXT_NAMESPACE "LoadingScreen"

void ULoadingScreen::ReleaseSlateResources(bool bReleaseChildren)
{
	LoadingSlate.Reset();
}

#if WITH_EDITOR
const FText ULoadingScreen::GetPaletteCategory()
{
	return LOCTEXT("CustomPaletteCategory", "Valorithm Slate");
}
#endif

TSharedRef<SWidget> ULoadingScreen::RebuildWidget()
{
	LoadingSlate = SNew(SLoadingScreen)
	.BackgroundBrush(&BackgroundBrush);
	return LoadingSlate.ToSharedRef();
}

void ULoadingScreen::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	LoadingSlate->SetBackgroundBrush(&BackgroundBrush);
}