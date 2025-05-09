// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamSelectAgentBox.generated.h"

class UTextBlock;
class UImage;
/**
 * 
 */
UCLASS()
class VALORANT_API UTeamSelectAgentBox : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ImageAgentThumb = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextStatus = nullptr;
	
	void ChangeAgentThumbImage(const int AgentId);
	void LockIn(int AgentId);
};