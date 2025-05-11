// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CreditComponent.generated.h"

class AAgentPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreditChanged, int32, NewCredit);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UCreditComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCreditComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// 크레딧 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Credits")
	FOnCreditChanged OnCreditChanged;

	// 현재 크레딧 얻기
	UFUNCTION(BlueprintPure, Category = "Credits")
	int32 GetCurrentCredit() const;

	// 최대 크레딧 얻기
	UFUNCTION(BlueprintPure, Category = "Credits")
	int32 GetMaxCredit() const;

	// 크레딧 추가 (킬, 라운드 승리 등)
	UFUNCTION(BlueprintCallable, Category = "Credits")
	void AddCredits(int32 Amount);

	// 크레딧 사용 (아이템 구매)
	UFUNCTION(BlueprintCallable, Category = "Credits")
	bool UseCredits(int32 Amount);
	
	// 특정 금액의 크레딧을 사용할 수 있는지 확인
	UFUNCTION(BlueprintCallable, Category = "Credits")
	bool CanUseCredits(int32 Amount) const;

	// 라운드 시작 시 크레딧 초기화
	UFUNCTION(BlueprintCallable, Category = "Credits")
	void ResetForNewRound();

	// 라운드 종료 시 보상 크레딧 지급
	UFUNCTION(BlueprintCallable, Category = "Credits")
	void AwardRoundEndCredits(bool bIsWinner, int32 ConsecutiveLosses);

	// 킬 획득 시 크레딧 보상
	UFUNCTION(BlueprintCallable, Category = "Credits")
	void AwardKillCredits();

	// 스파이크 설치/해제 크레딧 보상
	UFUNCTION(BlueprintCallable, Category = "Credits")
	void AwardSpikeActionCredits(bool bIsPlant);

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCredit, EditDefaultsOnly, Category = "Credits")
	int32 CurrentCredit;
	
	UPROPERTY(EditDefaultsOnly, Category = "Credits")
	int32 MaxCredit;
	
	UPROPERTY()
	AAgentPlayerState* OwnerState;

	// 크레딧 보상 상수들 - INI에서 로드되거나 데이터 테이블에서 가져올 수 있음
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Rewards")
	int32 KillReward;
	
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Rewards")
	int32 SpikeReward;
	
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Rewards")
	int32 RoundWinReward;
	
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Rewards")
	int32 RoundLossReward;
	
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Rewards")
	int32 ConsecutiveLossBonus;

private:
	// 서버에서 크레딧 변경 적용
	void SetCurrentCredit(int32 NewAmount);

	UFUNCTION()
	void OnRep_CurrentCredit();
}; 