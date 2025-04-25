// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MainMapWidget.h"
#include "MainMapLobbyPage.generated.h"

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
	
 	bool bIsFindingMatch = false;
	bool bIsHostingMatch = false;
	
	virtual void NativeConstruct() override;
	virtual void Init(UMainMapCoreUI* InitCoreUI) override;
	
	UFUNCTION(BlueprintCallable)
	void OnClickedButtonStart();

	void OnFindFirstSteamSessionComplete(const FOnlineSessionSearchResult& OnlineSessionSearchResult, bool bArg);
	void OnFindSteamSessionComplete(const TArray<FOnlineSessionSearchResult>& OnlineSessionSearchResults, bool bArg);
};