// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerState.h"

#include "Valorant.h"
#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Player/Agent/BaseAgent.h"
#include "Valorant/AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Component/CreditComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Net/UnrealNetwork.h"
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

void AAgentPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAgentPlayerState, m_AgentID);
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

	--(*Stack);
	OnAbilityStackChanged.Broadcast(AbilityID, *Stack);

	// 스택 변경 시 클라이언트들에게 동기화
	if (HasAuthority())
	{
		SyncAbilityStackToClients(AbilityID, *Stack);
	}

	return *Stack;
}

int32 AAgentPlayerState::AddAbilityStack(int32 AbilityID, int32 StacksToAdd)
{
	// 어빌리티 ID가 유효한지 확인
	if (AbilityID <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("유효하지 않은 어빌리티 ID: %d"), AbilityID);
		return 0;
	}

	// 최대 스택 제한 가져오기
	int32 MaxStack = GetMaxAbilityStack(AbilityID);

	// 스택 초기화 또는 증가
	int32* CurrentStack = AbilityStacks.Find(AbilityID);
	if (CurrentStack == nullptr)
	{
		// 새 항목 추가
		int32 NewStack = FMath::Min(StacksToAdd, MaxStack);
		AbilityStacks.Add(AbilityID, NewStack);
		OnAbilityStackChanged.Broadcast(AbilityID, NewStack);

		// 스택 변경 시 클라이언트들에게 동기화
		if (HasAuthority())
		{
			SyncAbilityStackToClients(AbilityID, NewStack);
		}

		return NewStack;
	}
	else
	{
		// 기존 항목 업데이트
		*CurrentStack = FMath::Min(*CurrentStack + StacksToAdd, MaxStack);
		OnAbilityStackChanged.Broadcast(AbilityID, *CurrentStack);

		// 스택 변경 시 클라이언트들에게 동기화
		if (HasAuthority())
		{
			SyncAbilityStackToClients(AbilityID, *CurrentStack);
		}

		return *CurrentStack;
	}
}

int32 AAgentPlayerState::SetAbilityStack(int32 AbilityID, int32 NewStack)
{
	// 어빌리티 ID가 유효한지 확인
	if (AbilityID <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("유효하지 않은 어빌리티 ID: %d"), AbilityID);
		return 0;
	}

	// 최대 스택 제한 가져오기
	int32 MaxStack = GetMaxAbilityStack(AbilityID);

	// 유효한 범위로 값 제한
	int32 ClampedStack = FMath::Clamp(NewStack, 0, MaxStack);

	// 스택 설정
	AbilityStacks.Add(AbilityID, ClampedStack);
	OnAbilityStackChanged.Broadcast(AbilityID, ClampedStack);

	// 스택 변경 시 클라이언트들에게 동기화
	if (HasAuthority())
	{
		SyncAbilityStackToClients(AbilityID, ClampedStack);
	}

	return ClampedStack;
}

bool AAgentPlayerState::HasAbilityStacks(int32 AbilityID) const
{
	return GetAbilityStack(AbilityID) > 0;
}

int32 AAgentPlayerState::GetMaxAbilityStack(int32 AbilityID) const
{
	// 기본 최대값 설정
	const int32 DefaultMaxStack = 2;

	// 게임 인스턴스가 없으면 기본값 반환
	if (!m_GameInstance)
	{
		return DefaultMaxStack;
	}

	// 어빌리티 데이터 가져오기
	FAbilityData* AbilityData = m_GameInstance->GetAbilityData(AbilityID);
	if (!AbilityData)
	{
		UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d에 대한 데이터를 찾을 수 없습니다."), AbilityID);
		return DefaultMaxStack;
	}

	// 데이터에 MaxStack 필드가 있다고 가정하고 값을 가져옴
	// 만약 필드가 없거나 0이하면 기본값 사용
	int32 MaxStack = AbilityData->MaxCharges;
	return (MaxStack > 0) ? MaxStack : DefaultMaxStack;
}

void AAgentPlayerState::OnKill()
{
	Muticast_OnKill();
}

void AAgentPlayerState::Muticast_OnKill_Implementation()
{
	OnKillDelegate.Broadcast();
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

void AAgentPlayerState::SetAgentID(int32 NewAgentID)
{
	m_AgentID = NewAgentID;
}

int32 AAgentPlayerState::GetAgentID() const
{
	return m_AgentID;
}

void AAgentPlayerState::SyncsAgentID_Implementation(int AgentID)
{
	m_AgentID = AgentID;
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

// 스택 변경 시 클라이언트에 동기화하는 헬퍼 함수
void AAgentPlayerState::SyncAbilityStackToClients(int32 AbilityID, int32 NewStack)
{
	if (HasAuthority())
	{
		// 자신의 클라이언트에게 동기화
		Client_SyncAbilityStack(AbilityID, NewStack);
	}
}

// 서버에서 클라이언트로 스택 변경 동기화
void AAgentPlayerState::Client_SyncAbilityStack_Implementation(int32 AbilityID, int32 NewStack)
{
	if (!HasAuthority())
	{
		// 클라이언트에서만 실행
		AbilityStacks.Add(AbilityID, NewStack);
		OnAbilityStackChanged.Broadcast(AbilityID, NewStack);
	}
}

// 서버에 스택 정보 요청
void AAgentPlayerState::Server_RequestAbilityStackSync_Implementation()
{
	if (HasAuthority())
	{
		// 서버에서 모든 스택 정보를 클라이언트로 전송
		TArray<int32> AbilityIDs;
		TArray<int32> Stacks;

		for (const TPair<int32, int32>& Pair : AbilityStacks)
		{
			AbilityIDs.Add(Pair.Key);
			Stacks.Add(Pair.Value);
		}

		Client_SyncAllAbilityStacks(AbilityIDs, Stacks);
	}
}

// 서버에서 모든 스택 정보 전송
void AAgentPlayerState::Client_SyncAllAbilityStacks_Implementation(const TArray<int32>& AbilityIDs,
                                                                   const TArray<int32>& Stacks)
{
	if (!HasAuthority())
	{
		// 클라이언트에서만 실행
		if (AbilityIDs.Num() == Stacks.Num())
		{
			// 로컬 스택 맵 초기화
			AbilityStacks.Empty();

			// 새 데이터로 채우기
			for (int32 i = 0; i < AbilityIDs.Num(); ++i)
			{
				AbilityStacks.Add(AbilityIDs[i], Stacks[i]);
				OnAbilityStackChanged.Broadcast(AbilityIDs[i], Stacks[i]);
			}
		}
	}
}
