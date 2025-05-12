// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ValorantObject/BaseInteractor.h"
#include "Spike.generated.h"

class ABaseAgent;
class AMatchGameState;
UENUM(BlueprintType)
enum class ESpikeState : uint8
{
	Dropped,    // 땅에 떨어진 상태
	Carried,    // 플레이어가 소지한 상태
	Planting,   // 설치 중인 상태
	Planted,    // 설치 완료된 상태
	Defusing,   // 해제 중인 상태
	Defused     // 해제 완료된 상태
};

UCLASS()
class VALORANT_API ASpike : public ABaseInteractor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ASpike();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 스파이크 현재 상태
	UPROPERTY(ReplicatedUsing=OnRep_SpikeState)
	ESpikeState SpikeState = ESpikeState::Dropped;

	// 스파이크 설치/해제 진행 시간
	UPROPERTY(Replicated)
	float InteractProgress = 0.0f;

	// 스파이크 설치 완료 후 폭발까지 남은 시간
	UPROPERTY(Replicated)
	float RemainingDetonationTime = 45.0f;

	// 스파이크 설치 필요 시간
	UPROPERTY(EditDefaultsOnly, Category = "Spike")
	float PlantTime = 4.0f;

	// 스파이크 해제 필요 시간
	UPROPERTY(EditDefaultsOnly, Category = "Spike")
	float DefuseTime = 7.0f;

	// 반 해제 필요 시간 (기본 해제 시간의 절반)
	UPROPERTY(EditDefaultsOnly, Category = "Spike")
	float HalfDefuseTime = 3.5f;

	// 현재 상호작용 중인 에이전트 (설치 또는 해제 중인 에이전트)
	UPROPERTY(Replicated)
	ABaseAgent* InteractingAgent = nullptr;

	// 스파이크 사이트 위치 확인용 (A, B, C 사이트)
	UPROPERTY(EditAnywhere, Category = "Spike")
	FString SiteName = "Unknown";

	// 스파이크 설치 위치
	FVector PlantingLocation;

	// 스파이크 상태 변경 시 호출되는 함수
	UFUNCTION()
	void OnRep_SpikeState();
	
	// 스파이크가 반 해제되었는지 여부
	UPROPERTY(Replicated)
	bool bIsHalfDefused = false;
	
	// 마지막으로 해제를 시도한 에이전트
	UPROPERTY(Replicated)
	ABaseAgent* LastDefusingAgent = nullptr;
	
	// 캐싱된 게임 스테이트 참조
	UPROPERTY()
	AMatchGameState* CachedGameState = nullptr;

public:
	virtual void ServerRPC_PickUp_Implementation(ABaseAgent* Agent) override;
	virtual void ServerRPC_Drop_Implementation() override;
	virtual void ServerRPC_Interact_Implementation(ABaseAgent* InteractAgent) override;

	// 스파이크 설치/해제 취소 - ToDo : 죽을때 호출해주기
	UFUNCTION(Server, Reliable)
	void ServerRPC_Cancel(ABaseAgent* InteractAgent);

	// 스파이크 설치 시작
	UFUNCTION(Server, Reliable)
	void ServerRPC_StartPlanting(ABaseAgent* Agent);

	// 스파이크 설치 취소
	UFUNCTION(Server, Reliable)
	void ServerRPC_CancelPlanting();

	// 스파이크 설치 완료
	UFUNCTION(Server, Reliable)
	void ServerRPC_FinishPlanting();

	// 스파이크 해제 시작
	UFUNCTION(Server, Reliable)
	void ServerRPC_StartDefusing(ABaseAgent* Agent);

	// 스파이크 해제 취소
	UFUNCTION(Server, Reliable)
	void ServerRPC_CancelDefusing();

	// 스파이크 해제 완료
	UFUNCTION(Server, Reliable)
	void ServerRPC_FinishDefusing();

	// 스파이크 폭발
	UFUNCTION(Server, Reliable)
	void ServerRPC_Detonate();

	// 스파이크 상태 반환
	UFUNCTION(BlueprintCallable, Category = "Spike")
	ESpikeState GetSpikeState() const { return SpikeState; }

	// 스파이크 설치/해제 진행도 반환
	UFUNCTION(BlueprintCallable, Category = "Spike")
	float GetInteractProgress() const { return InteractProgress; }

	// 스파이크 폭발까지 남은 시간 반환
	UFUNCTION(BlueprintCallable, Category = "Spike")
	float GetRemainingDetonationTime() const { return RemainingDetonationTime; }
	
	// 현재 스파이크와 상호작용 중인 에이전트 반환
	UFUNCTION(BlueprintCallable, Category = "Spike")
	ABaseAgent* GetInteractingAgent() const { return InteractingAgent; }
	
	// 반 해제 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Spike")
	bool IsHalfDefused() const { return bIsHalfDefused; }

	virtual void Destroyed() override;

protected:
	virtual bool ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const override;
	virtual bool ServerOnly_CanDrop() const override;
	virtual bool ServerOnly_CanInteract() const override;

	// 스파이크 설치 가능한 위치인지 확인
	bool IsInPlantZone() const;
	
	// 현재 게임 상태가 InRound인지 확인
	bool IsGameStateInRound() const;

	// 효과음 재생 등을 위한 이벤트 함수
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnPlantingStarted();

	UFUNCTION(BlueprintImplementableEvent)
	void HandlePlantingStarted();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnPlantingCancelled();

	UFUNCTION(BlueprintImplementableEvent)
	void HandlePlantingCancelled();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnPlantingFinished();

	UFUNCTION(BlueprintImplementableEvent)
	void HandlePlantingFinished();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnDefusingStarted(bool bHalfDefuse);

	UFUNCTION(BlueprintImplementableEvent)
	void HandleDefusingStarted();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnDefusingCancelled();
	
	UFUNCTION(BlueprintImplementableEvent)
	void HandleDefusingCancelled();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnDefusingFinished();

	UFUNCTION(BlueprintImplementableEvent)
	void HandleDefusingFinished();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnDetonated();

	UFUNCTION(BlueprintImplementableEvent)
	void HandleDetonated();
	
	// 해제 진행도 체크하여 반 해제 여부 설정
	void CheckHalfDefuse();
};
