// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "AgentSelectButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentSelectButtonClicked, int, AgentId);

/**
 * 
 */
UCLASS()
class VALORANT_API UAgentSelectButton : public UButton
{
	GENERATED_BODY()

public:
	FOnAgentSelectButtonClicked OnAgentSelectButtonClicked;

	void Init(const int InitAgentId)
	{
		AgentId = InitAgentId;
	}

protected:
	int AgentId = 0;
	
	virtual void SynchronizeProperties() override
	{
		Super::SynchronizeProperties();
		OnClicked.Clear();
		OnClicked.AddDynamic(this, &UAgentSelectButton::InternalOnClicked);
	}

	UFUNCTION()
	void InternalOnClicked()
	{
		OnAgentSelectButtonClicked.Broadcast(AgentId); // 자신을 넘김
	}
};
