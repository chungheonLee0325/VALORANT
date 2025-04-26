// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMapCoreUI.h"

#include "MainMapWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetSwitcher.h"

void UMainMapCoreUI::NativeConstruct()
{
	Super::NativeConstruct();

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (auto* FoundWidget : Widgets)
	{
		if (auto* MainMapWidget = Cast<UMainMapWidget>(FoundWidget))
		{
			MainMapWidget->Init(this);
		}
	}
}

void UMainMapCoreUI::SwitchPage(const EMainMapPage& DestinationPage)
{
	WidgetSwitcherPage->SetActiveWidgetIndex(static_cast<int>(DestinationPage));
}
