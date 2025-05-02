// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MatchPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectedAgentChanged, const FString&, PlayerNickname, int, SelectedAgentID);

/**
 * 
 */
UCLASS()
class VALORANT_API AMatchPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated)
	FString DisplayName = "None";
	UPROPERTY(Replicated)
	bool bIsBlueTeam = true;
	UPROPERTY(ReplicatedUsing=OnRep_SelectedAgentID)
	int32 SelectedAgentID = 0;
	UPROPERTY(BlueprintAssignable)
	FOnSelectedAgentChanged OnSelectedAgentChanged;

	UFUNCTION(Server, Reliable)
	void ServerRPC_NotifyAgentSelected(int AgentId);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_SelectedAgentID();
};
