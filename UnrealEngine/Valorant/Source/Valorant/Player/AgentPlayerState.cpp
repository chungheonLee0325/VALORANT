// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerState.h"

#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Player/Agent/BaseAgent.h"
#include "Valorant/AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Valorant/GameManager/ValorantGameInstance.h"

AAgentPlayerState::AAgentPlayerState()
{
	ASC = CreateDefaultSubobject<UAgentAbilitySystemComponent>(TEXT("ASC"));
	ASC-> SetIsReplicated(true);

	// GE에 의해 값이 변경될 때, GE 인스턴스를 복제할 범위 설정
	// Full - 항상 복제 / Minimal - GE에 의해 변경되는 AttributeData 값만 복제 / Mixed - 혼합
	// AttributeData에 ReplicatedUsing 설정을 하는 것과는 별개의 개념.
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));

	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
}

int32 AAgentPlayerState::GetAbilityStack(int32 AbilityID) const
{
	const int32* Stack = AbilityStacks.Find(AbilityID);
	return Stack ? *Stack : 0;
}

int32 AAgentPlayerState::ReduceAbilityStack(int32 AbilityID)
{
	int32* Stack = AbilityStacks.Find(AbilityID);
	if (Stack == nullptr || *Stack == 0)
	{
		return 0;
	}
	return AbilityStacks[AbilityID]--;
}

void AAgentPlayerState::BeginPlay()
{
	Super::BeginPlay();

	m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
}

UAbilitySystemComponent* AAgentPlayerState::GetAbilitySystemComponent() const
{
	return ASC;
}

UBaseAttributeSet* AAgentPlayerState::GetBaseAttributeSet() const
{
	return BaseAttributeSet;
}
float AAgentPlayerState::GetHealth() const
{
	return BaseAttributeSet->GetHealth();
}

float AAgentPlayerState::GetMaxHealth() const
{
	return BaseAttributeSet->GetMaxHealth();
}

float AAgentPlayerState::GetArmor() const
{
	return BaseAttributeSet->GetArmor();
}

float AAgentPlayerState::GetMoveSpeed() const
{
	return BaseAttributeSet->GetMoveSpeed();
}

void AAgentPlayerState::Server_PurchaseAbility_Implementation(int32 AbilityID)
{
	if (!GetAbilitySystemComponent() || !GetBaseAttributeSet()) return;

	FAbilityData* AbilityData = m_GameInstance->GetAbilityData(AbilityID);
	// 1. 비용 확인
	int32 Cost = AbilityData->ChargeCost;

	if (CurrentCredit >= Cost)
	{
		// 2. 비용 차감 
		CurrentCredit -= Cost;

		// 3. 어빌리티 부여
		// FGameplayAbilitySpec AbilitySpec(AbilityData->AbilityClass, 1, INDEX_NONE, this); // Level 1, InputID 없음
		// //AbilitySpec.Ability;
		// GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
		
		
		int32* Stack = AbilityStacks.Find(AbilityID);
		if (Stack == nullptr)
		{
			AbilityStacks.Add(AbilityID, 0);
		}
		AbilityStacks[AbilityID]++;

		// TODO: 클라이언트에 구매 성공/실패 피드백 (RPC 또는 Replicated 변수 사용)
		UE_LOG(LogTemp, Log, TEXT("Player %s purchased skill %s"), *GetName(), *AbilityData->AbilityName);
	}
	else
	{
		// TODO: 클라이언트에 구매 실패 피드백 (크레딧 부족)
		UE_LOG(LogTemp, Log, TEXT("Player %s failed to purchase skill %s: Insufficient credits"), *GetName(), *AbilityData->AbilityName);
	}
}

bool AAgentPlayerState::Server_PurchaseAbility_Validate(int32 SkillID)
{
	return SkillID != 0;
}



