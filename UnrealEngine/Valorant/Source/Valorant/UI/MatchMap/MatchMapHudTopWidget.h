// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapHudTopWidget.generated.h"

enum class ERoundSubState : uint8;
struct FKillFeedInfo;
class ABaseAgent;
class AAgentPlayerState;
class AMatchGameState;
class UCanvasPanel;
class UImage;
class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapHudTopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindMatchState(AMatchGameState* Match);

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockTime = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockBlueScore = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRedScore = nullptr;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanelTimer = nullptr;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UImage> ImageSpike = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TMap<AAgentPlayerState*, UUserWidget*> PlayerCardMap;
	
	UFUNCTION()
	void UpdateTime(float Time);
	UFUNCTION()
	void UpdateScore(int TeamBlueScore, int TeamRedScore);

	UFUNCTION()
	void OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime);
	UFUNCTION()
	void OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& Info);
	UFUNCTION(BlueprintImplementableEvent)
	void MarkKillOnPlayerCard(const AAgentPlayerState* VictimPS);
	UFUNCTION(BlueprintImplementableEvent)
	void InitPlayerCard();

protected:
	virtual void NativeDestruct() override;

private:
	TWeakObjectPtr<AMatchGameState> BoundMatchState;
	TOptional<ERoundSubState> LastPhase;
	int32 LastDisplayedSecond = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Style")
	FLinearColor BuyClockColor = FLinearColor(0.62f, 0.90f, 0.91f);
};
