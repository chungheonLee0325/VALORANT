// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MatchGameState.generated.h"

enum class ERoundSubState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemainRoundStateTimeChanged, float, Time);

/**
 * 
 */
UCLASS()
class VALORANT_API AMatchGameState : public AGameState
{
	GENERATED_BODY()

protected:
	UPROPERTY(ReplicatedUsing=OnRep_RoundSubState)
	ERoundSubState RoundSubState;

	UPROPERTY(ReplicatedUsing=OnRep_RemainRoundStateTime)
	float RemainRoundStateTime = 0.0f;

public:
	FRemainRoundStateTimeChanged OnRemainRoundStateTimeChanged;
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*
	 *	MatchState 관련 Handle
	 */
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void HandleLeavingMap() override;
	
	/*
	 *	RoundSubState 관련 Handle
	 */
	UFUNCTION()
	void OnRep_RoundSubState();
	UFUNCTION()
	void OnRep_RemainRoundStateTime();
	void HandleRoundSubState_SelectAgent();
	void HandleRoundSubState_PreRound();
	void HandleRoundSubState_InRound();
	void HandleRoundSubState_EndRound();

public:
	void SetRoundSubState(ERoundSubState NewRoundSubState);
	void SetRemainRoundStateTime(float NewRemainRoundStateTime);
};
