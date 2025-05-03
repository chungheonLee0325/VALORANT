// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapSelectAgentUI.generated.h"

class UTeamSelectAgentBox;
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
	void OnSelectedAgentChanged(const FString& DisplayName, int SelectedAgentID);
	
	UFUNCTION()
	void UpdateTime(float Time);
	void FillAgentList();
	UFUNCTION(BlueprintImplementableEvent)
	void AddTeamBox(const FString& DisplayName);
	

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess))
	TMap<FString, UTeamSelectAgentBox*> TeamSelectAgentBoxMap;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UGridPanel> GridPanelAgentList = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRemTime = nullptr;

public:
	void FillTeamSelectAgentList(const TArray<FString>& TeamPlayerNameArray);
};
