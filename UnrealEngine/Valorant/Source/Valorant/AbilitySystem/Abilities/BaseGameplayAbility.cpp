// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameplayAbility.h"

#include <GameManager/SubsystemSteamManager.h>

#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Player/AgentPlayerState.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UBaseGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
	// 기본 활성화 조건 확인
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	//어빌리티 ID가 유효하고 스택 소비가 필요한 경우
	if (m_AbilityID > 0)
	{
		// 플레이어 스테이트 가져오기
		AAgentPlayerState* PS = Cast<AAgentPlayerState>(ActorInfo->PlayerController->PlayerState);
		if (PS == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("어빌리티 활성화 확인 실패: PlayerState가 NULL입니다."));
			return false;
		}

		// 현재 스택 확인
		int32 CurrentStack = PS->GetAbilityStack(m_AbilityID);
		CurrentStack = GetAbilityStack(ActorInfo->PlayerController.Get());

		// 스택이 없으면 활성화 불가
		if (CurrentStack <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 없어 활성화할 수 없습니다."), m_AbilityID);
			return false;
		}
	}

	return true;
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	// 이미 스택 확인은 CanActivateAbility에서 했으므로 여기서 스택 소비
	if (m_AbilityID > 0)
	{
		// 스택 소비 시도
		// 참고: 직접 ConsumeAbilityStack을 호출하지 않고, 실제 로직에서 적절한 시점에 호출하도록 함
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// NET_LOG(LogTemp,Warning,TEXT("액티배이트 어빌리티"));

	// ToDO : 삭제 고민 Test
	m_ActorInfo = *ActorInfo;

	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		if (FollowUpInputTags.IsEmpty() == false)
		{
			asc->SetSkillReady(false);
			asc->SetSkillClear(false);
			asc->ResisterFollowUpInput(FollowUpInputTags);
		}
		else
		{
			Active_General();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GA, asc가 AgentAbilitySystemComponent를 상속받지 않았어요."));
	}
}

void UBaseGameplayAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	NET_LOG(LogTemp, Warning, TEXT("스킬 InputPressed"));
}

void UBaseGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	NET_LOG(LogTemp, Warning, TEXT("스킬 InputReleased"));
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	NET_LOG(LogTemp, Warning, TEXT("스킬 EndAbility"));

	// CurrentFollowUpInputTag = FGameplayTag();
	if (!bWasCancelled)
	{
		ConsumeAbilityStack(ActorInfo->PlayerController.Get());
	}

	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		asc->SetSkillClear(true);
	}
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         bool bReplicateCancelAbility)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
	NET_LOG(LogTemp, Warning, TEXT("스킬 CancelAbility"));

	// CurrentFollowUpInputTag = FGameplayTag();

	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		asc->SetSkillClear(true);
	}
}

void UBaseGameplayAbility::Active_General()
{
}

void UBaseGameplayAbility::Active_Left_Click(FGameplayEventData data)
{
}

void UBaseGameplayAbility::Active_Right_Click(FGameplayEventData data)
{
}


void UBaseGameplayAbility::SetAbilityID(int32 AbilityID)
{
	m_AbilityID = AbilityID;
}

// 어빌리티 스택 감소 메서드 구현
bool UBaseGameplayAbility::ConsumeAbilityStack(const APlayerController* PlayerController)
{
	// 플레이어 스테이트 가져오기
	AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
	if (PS == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 감소 실패: PlayerState가 NULL입니다."));
		return false;
	}

	// 현재 스택 가져오기
	int32 CurrentStack = PS->GetAbilityStack(m_AbilityID);

	// 스택이 있으면 감소
	if (CurrentStack > 0)
	{
		PS->ReduceAbilityStack(m_AbilityID);
		UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 감소됨. 남은 스택: %d"), m_AbilityID, CurrentStack - 1);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 없습니다."), m_AbilityID);
	return false;
}

// 어빌리티 스택 확인 메서드 구현
int32 UBaseGameplayAbility::GetAbilityStack(const APlayerController* PlayerController) const
{
	// 플레이어 스테이트 가져오기
	AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
	if (PS == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 확인 실패: PlayerState가 NULL입니다."));
		return 0;
	}

	// 현재 스택 반환
	return PS->GetAbilityStack(m_AbilityID);
}
