// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapSelectAgentUI.generated.h"

class UTeamSelectAgentBox;
class UGridPanel;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClickAgentSelectButtonDelegate, int, SelectedAgentID);

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
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockPosition1 = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockAgentName = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockAgentDescription = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockPosition2 = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockPositionDescription = nullptr;
	
public:
	FOnClickAgentSelectButtonDelegate OnClickAgentSelectButtonDelegate;

	UPROPERTY()
	int32 CurrentSelectedAgentID = 0;
	
	void FillTeamSelectAgentList(const TArray<FString>& TeamPlayerNameArray);
	void OnSelectedAgentChanged(const FString& DisplayName, int SelectedAgentID);
	void OnLockIn(const FString& DisplayName, const int AgentId);
};
