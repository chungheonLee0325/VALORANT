// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyPageUI.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API ULobbyPageUI : public UUserWidget
{
	GENERATED_BODY()

private:
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UUserWidget> MainMapUI = nullptr;
	bool bIsFindingMatch = false;
	
public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void FindMatch();

	void JoinMatch(const FOnlineSessionSearchResult& OnlineSessionSearchResult, bool bArg);
};
