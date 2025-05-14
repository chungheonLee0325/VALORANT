// Fill out your copyright notice in the Description page of Project Settings.


#include "CreditComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/AgentPlayerState.h"

UCreditComponent::UCreditComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// 기본값 설정
	CurrentCredit = 800; // 발로란트 첫 라운드 시작 크레딧
	MaxCredit = 9000;
	
	// 보상 크레딧 기본값 설정
	KillReward = 200;
	SpikeReward = 300;
	RoundWinReward = 3000;
	RoundLossReward = 1900;
	ConsecutiveLossBonus = 500;

#ifdef DEBUGTEST
	// 기본값 설정
	CurrentCredit += 3000; // 발로란트 첫 라운드 시작 크레딧
	MaxCredit += 9000;
	
	// 보상 크레딧 기본값 설정
	KillReward += 200;
	SpikeReward += 300;
	RoundWinReward += 3000;
	RoundLossReward += 1900;
	ConsecutiveLossBonus += 500;
#endif
	// 컴포넌트 리플리케이션 활성화
	SetIsReplicatedByDefault(true);
}

void UCreditComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// CurrentCredit을 리플리케이션 설정
	DOREPLIFETIME(UCreditComponent, CurrentCredit);
}

void UCreditComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 소유 PlayerState 찾기
	AActor* Owner = GetOwner();
	if (Owner)
	{
		OwnerState = Cast<AAgentPlayerState>(Owner);
	}
}

int32 UCreditComponent::GetCurrentCredit() const
{
	return CurrentCredit;
}

int32 UCreditComponent::GetMaxCredit() const
{
	return MaxCredit;
}

void UCreditComponent::AddCredits(int32 Amount)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		const int32 NewAmount = FMath::Clamp(CurrentCredit + Amount, 0, MaxCredit);
		SetCurrentCredit(NewAmount);
	}
}

bool UCreditComponent::UseCredits(int32 Amount)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (CurrentCredit >= Amount)
		{
			const int32 NewAmount = CurrentCredit - Amount;
			SetCurrentCredit(NewAmount);
			return true;
		}
	}
	return false;
}

bool UCreditComponent::CanUseCredits(int32 Amount) const
{
	return CurrentCredit >= Amount;
}

void UCreditComponent::ResetForNewRound()
{
	// 라운드 시작 시 기존 크레딧을 유지
}

void UCreditComponent::AwardRoundEndCredits(bool bIsWinner, int32 ConsecutiveLosses)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		int32 CreditAward = bIsWinner ? RoundWinReward : RoundLossReward;
		
		// 연속 패배 보너스 적용
		if (!bIsWinner && ConsecutiveLosses > 0)
		{
			CreditAward += FMath::Min(ConsecutiveLosses, 3) * ConsecutiveLossBonus;
		}
		
		AddCredits(CreditAward);
	}
}

void UCreditComponent::AwardKillCredits()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		int32 Award = KillReward;
		AddCredits(Award);
	}
}

void UCreditComponent::AwardSpikeActionCredits(bool bIsPlant)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		AddCredits(SpikeReward);
	}
}

void UCreditComponent::SetCurrentCredit(int32 NewAmount)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		const int32 OldCredit = CurrentCredit;
		CurrentCredit = FMath::Clamp(NewAmount, 0, MaxCredit);
		
		if (OldCredit != CurrentCredit)
		{
			// 크레딧 변경 이벤트 발생
			OnCreditChanged.Broadcast(CurrentCredit);
		}
	}
}

void UCreditComponent::OnRep_CurrentCredit()
{
	// 클라이언트에서 CurrentCredit 값이 복제될 때 호출됨
	// UI 업데이트 등을 위해 델리게이트 호출
	OnCreditChanged.Broadcast(CurrentCredit);
} 