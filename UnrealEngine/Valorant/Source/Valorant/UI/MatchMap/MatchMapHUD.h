// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameManager/MatchGameState.h"
#include "MatchMapHUD.generated.h"

class UWidgetSwitcher;
class UTextBlock;

UENUM(BlueprintType)
enum class EMatchAnnouncement : uint8
{
	EMA_Won,
	EMA_Lost,
	EMA_BuyPhase
};

/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	UFUNCTION()
	void UpdateTime(float Time);
	UFUNCTION()
	void UpdateScore(int TeamBlueScore, int TeamRedScore);
	UFUNCTION()
	void OnRoundSubStateChanged(const ERoundSubState RoundSubState);

	FTimerHandle AnnouncementTimerHandle;
	UFUNCTION()
	void DisplayAnnouncement(EMatchAnnouncement MatchAnnouncement, const float DisplayTime);
	void HideAnnouncement();
	
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockTime = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockBlueScore = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRedScore = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcherAnnouncement = nullptr;
	
/*
 *	Debug
 */
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRoundSubStateDbg = nullptr;

protected:
	UFUNCTION()
	void DebugRoundSubState(const FString& RoundSubStateStr);
};
