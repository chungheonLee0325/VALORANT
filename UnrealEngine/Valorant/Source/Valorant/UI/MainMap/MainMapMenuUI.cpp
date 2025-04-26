// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMapMenuUI.h"

#include "MainMapCoreUI.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMainMapMenuUI::NativeConstruct()
{
	Super::NativeConstruct();
}

void UMainMapMenuUI::SetTitle(const FString& Title)
{
	if (Title == TEXT("로비"))
	{
		ButtonPlay->SetIsEnabled(false);
	}
	
	if (TextTitle)
	{
		TextTitle->SetText(FText::FromString(Title));
	}
}

void UMainMapMenuUI::OnClickedButtonBack()
{
	if (CoreUI)
	{
		CoreUI->SwitchPage(EMainMapPage::MainMapHomePage);
	}
}

void UMainMapMenuUI::OnClickedButtonPlay()
{
	if (TextTitle->GetText().ToString() == "로비")
	{
		return;
	}

	CoreUI->SwitchPage(EMainMapPage::MainMapLobbyPage);
}