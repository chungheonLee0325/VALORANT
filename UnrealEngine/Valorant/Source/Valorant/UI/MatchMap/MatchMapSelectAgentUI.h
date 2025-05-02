// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapSelectAgentUI.generated.h"

class UTeamSelectAgentBox;
class UHorizontalBox;
class UGridPanel;
class UTextBlock;
/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapSelectAgentUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	void OnClickedButtonLockIn();
	UFUNCTION()
	void OnClickedAgentSelectButton(int AgentId);
	UFUNCTION()
	void OnSelectedAgentChanged(const FString& PlayerNickname, int SelectedAgentID);
	
	UFUNCTION()
	void UpdateTime(float Time);
	void FillAgentList();
	void FillTeamList();

	UPROPERTY()
	TMap<FString, UTeamSelectAgentBox*> TeamSelectAgentBoxMap;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UGridPanel> GridPanelAgentList = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRemTime = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> HorizontalBoxTeamList = nullptr;
};
