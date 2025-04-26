// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMapCoreUI.generated.h"

class UMainMapHomePage;
class UWidgetSwitcher;
class UMainMapLobbyPage;

UENUM(BlueprintType)
enum class EMainMapPage : uint8
{
	MainMapHomePage UMETA(DisplayName = "MainMapHomePage"),
	MainMapLobbyPage UMETA(DisplayName = "MainMapLobbyPage"),
};

/**
 * 
 */
UCLASS()
class VALORANT_API UMainMapCoreUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
public:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcherPage = nullptr;

	UFUNCTION(BlueprintCallable)
	void SwitchPage(const EMainMapPage& DestinationPage);
};
