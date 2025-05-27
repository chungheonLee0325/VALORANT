// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MatchGameMode.h"
#include "GameFramework/GameState.h"
#include "MatchGameState.generated.h"

enum class ERoundSubState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemainRoundStateTimeChanged, float, Time);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTeamScoreChanged, int, TeamBlueScore, int, TeamRedScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRoundSubStateChanged, const ERoundSubState, RoundSubState, const float, TransitionTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRoundEnd, bool, bBlueWin, const ERoundEndReason, RoundEndReason, const float, TransitionTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpikePlanted, AMatchPlayerController*, Planter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpikeDefused, AMatchPlayerController*, Defuser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShift);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEnd, const bool, bBlueWin);

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

	UPROPERTY(ReplicatedUsing=OnRep_TeamScore)
	int TeamBlueScore = 0;
	UPROPERTY(ReplicatedUsing=OnRep_TeamScore)
	int TeamRedScore = 0;

public:
	FRemainRoundStateTimeChanged OnRemainRoundStateTimeChanged;
	FTeamScoreChanged OnTeamScoreChanged;
	FRoundSubStateChanged OnRoundSubStateChanged;
	FOnRoundEnd OnRoundEnd;
	// 상점 닫기 이벤트
	UPROPERTY(BlueprintAssignable, Category="Shop")
	FOnShopClosed OnShopClosed;
	// 스파이크 이벤트
	UPROPERTY(BlueprintAssignable, Category="Spike")
	FOnSpikePlanted OnSpikePlanted;
	UPROPERTY(BlueprintAssignable, Category="Spike")
	FOnSpikeDefused OnSpikeDefused;
	FOnShift OnShift;
	FOnMatchEnd OnMatchEnd;
	
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
	UPROPERTY(Replicated)
	float TransitionTime = 0.0f;
	UFUNCTION()
	void OnRep_RoundSubState();
	UFUNCTION()
	void OnRep_RemainRoundStateTime();
	void HandleRoundSubState_SelectAgent();
	void HandleRoundSubState_PreRound();
	void HandleRoundSubState_BuyPhase();
	void HandleRoundSubState_InRound();
	void HandleRoundSubState_EndRound();

	UFUNCTION()
	void OnRep_TeamScore();

public:
	void SetRoundSubState(ERoundSubState NewRoundSubState, float NewTransitionTime);
	void SetRemainRoundStateTime(float NewRemainRoundStateTime);
	UFUNCTION(NetMultiCast, Reliable)
	void MulticastRPC_HandleRoundEnd(bool bBlueWin, ERoundEndReason RoundEndReason);
	void SetTeamScore(int NewTeamBlueScore, int NewTeamRedScore);
	
	// 현재 상점을 열 수 있는 상태인지 확인 (클라이언트에서도 호출 가능)
	UFUNCTION(BlueprintCallable, Category="Shop")
	bool CanOpenShop() const;

	// 현재 라운드 상태 가져오기
	UFUNCTION(BlueprintCallable, Category="Round")
	ERoundSubState GetRoundSubState() const;

	// 모든 클라이언트의 상점 닫기 요청 브로드캐스트
	UFUNCTION(NetMulticast, Reliable, Category="Shop")
	void MulticastRPC_CloseAllShops();

	// 스파이크 설치 이벤트 브로드캐스트
	UFUNCTION(NetMulticast, Reliable, Category="Spike")
	void MulticastRPC_OnSpikePlanted(AMatchPlayerController* Planter);

	// 스파이크 해제 이벤트 브로드캐스트
	UFUNCTION(NetMulticast, Reliable, Category="Spike")
	void MulticastRPC_OnSpikeDefused(AMatchPlayerController* Defuser);

	UFUNCTION(NetMultiCast, Reliable)
	void MulticastRPC_OnShift();

	UFUNCTION(NetMultiCast, Reliable)
	void MulticastRPC_OnMatchEnd(const bool bBlueWin);
};
