// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerState.h"

#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Player/Agent/BaseAgent.h"
#include "Valorant/AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Component/CreditComponent.h"
#include "Valorant/Player/AgentPlayerController.h"

AAgentPlayerState::AAgentPlayerState()
{
	ASC = CreateDefaultSubobject<UAgentAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);

	// GE에 의해 값이 변경될 때, GE 인스턴스를 복제할 범위 설정
	// Full - 항상 복제 / Minimal - GE에 의해 변경되는 AttributeData 값만 복제 / Mixed - 혼합
	// AttributeData에 ReplicatedUsing 설정을 하는 것과는 별개의 개념.
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));

	// 크레딧 컴포넌트 생성 및 초기화
	CreditComponent = CreateDefaultSubobject<UCreditComponent>(TEXT("CreditComponent"));

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

	// 크레딧 변경 이벤트를 BP 및 다른 컴포넌트에 브로드캐스트하기 위한 델리게이트 설정
	if (CreditComponent)
	{
		CreditComponent->OnCreditChanged.AddDynamic(this, &AAgentPlayerState::OnCreditChanged);
	}
}

// 크레딧 변경 이벤트 핸들러
void AAgentPlayerState::OnCreditChanged(int32 NewCredit)
{
	// PlayerState에서 크레딧 변경 이벤트를 필요한 시스템에 전달
	// 여기서는 OnCreditChanged 델리게이트만 브로드캐스트하고
	// 실제 처리는 크레딧 컴포넌트에서 처리
	OnCreditChangedDelegate.Broadcast(NewCredit);
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

float AAgentPlayerState::GetEffectSpeed() const
{
	return BaseAttributeSet->GetEffectSpeedMultiplier();
}

void AAgentPlayerState::Server_PurchaseAbility_Implementation(int32 AbilityID)
{
	if (!GetAbilitySystemComponent() || !GetBaseAttributeSet() || !CreditComponent) return;

	FAbilityData* AbilityData = m_GameInstance->GetAbilityData(AbilityID);
	if (!AbilityData) return;

	// 1. 비용 확인
	int32 Cost = AbilityData->ChargeCost;

	// 크레딧 컴포넌트를 통한 구매 시도
	if (CreditComponent->CanUseCredits(Cost) && CreditComponent->UseCredits(Cost))
	{
		// 2. 어빌리티 스택 증가
		int32* Stack = AbilityStacks.Find(AbilityID);
		if (Stack == nullptr)
		{
			AbilityStacks.Add(AbilityID, 0);
		}
		AbilityStacks[AbilityID]++;

		// 클라이언트에 구매 성공/실패 피드백 
		UE_LOG(LogTemp, Log, TEXT("Player %s purchased skill %s"), *GetName(), *AbilityData->AbilityName);
		AAgentPlayerController* PC = GetOwner<AAgentPlayerController>();
		if (PC)
		{
			PC->Client_ReceivePurchaseResult(true, AbilityID, EShopItemType::Ability, TEXT(""));
		}
	}
	else
	{
		// 클라이언트에 구매 실패 피드백 (크레딧 부족)
		UE_LOG(LogTemp, Log, TEXT("Player %s failed to purchase skill %s: Insufficient credits"), *GetName(),
		       *AbilityData->AbilityName);
		AAgentPlayerController* PC = GetOwner<AAgentPlayerController>();
		if (PC)
		{
			PC->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                 TEXT("Insufficient credits for ability."));
		}
	}
}

bool AAgentPlayerState::Server_PurchaseAbility_Validate(int32 SkillID)
{
	return SkillID != 0;
}

int32 AAgentPlayerState::GetCurrentCredit() const
{
	if (CreditComponent)
	{
		return CreditComponent->GetCurrentCredit();
	}
	return 0;
}

// 클라이언트에서 서버에 크레딧 동기화 요청
void AAgentPlayerState::Server_RequestCreditSync_Implementation()
{
	// 서버에서 실행될 때 현재 크레딧 정보를 클라이언트에 전송
	if (HasAuthority() && CreditComponent)
	{
		int32 CurrentCredit = CreditComponent->GetCurrentCredit();
		// 명시적인 크레딧 값 전달
		Client_SyncCredit(CurrentCredit);
	}
}

// 추가: 서버에서 클라이언트로 크레딧 정보 전송
void AAgentPlayerState::Client_SyncCredit_Implementation(int32 SyncedCredit) // 파라미터 추가
{
	// 클라이언트에서 실행될 때 이벤트 발생
	if (!HasAuthority())
	{
		// 또는 OnCreditChanged 함수를 직접 호출하여 내부 로직 처리 및 델리게이트 호출
		OnCreditChanged(SyncedCredit);
	}
	// else if (HasAuthority() && CreditComponent)
	// {
	// 	// 서버에서도 UI 업데이트를 위해 델리게이트 호출
	// 	OnCreditChanged(SyncedCredit);
	// }
}
