// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MainMapWidget.h"
#include "MainMapLobbyPage.generated.h"

class UButton;
class UWidgetSwitcher;
class UMainMapMenuUI;
class UMainMapCoreUI;
/**
 * 
 */
UCLASS()
class VALORANT_API UMainMapLobbyPage : public UMainMapWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UMainMapMenuUI> MenuUI = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> ButtonStart = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsProgressMatchMaking = false;
 	bool bIsFindingSession = false;
	bool bIsHostingSession = false;
	
	virtual void NativeConstruct() override;
	virtual void Init(UMainMapCoreUI* InitCoreUI) override;
	
	UFUNCTION(BlueprintCallable)
	void OnClickedButtonStart();

	UFUNCTION(BlueprintCallable)
	void OnClickedButtonCancel();

	void OnFindFirstSteamSessionComplete(const FOnlineSessionSearchResult& OnlineSessionSearchResult, bool bArg);
	void OnFindSteamSessionComplete(const TArray<FOnlineSessionSearchResult>& OnlineSessionSearchResults, bool bArg);
};