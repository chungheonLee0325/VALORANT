// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMapHomePage.h"

#include "MainMapCoreUI.h"

void UMainMapHomePage::NativeConstruct()
{
	Super::NativeConstruct();
}

void UMainMapHomePage::OnClickedButtonPlay()
{
	if (CoreUI)
	{
		CoreUI->SwitchPage(EMainMapPage::MainMapLobbyPage);
	}
}