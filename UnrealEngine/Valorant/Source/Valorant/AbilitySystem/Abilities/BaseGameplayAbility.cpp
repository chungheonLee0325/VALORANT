// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameplayAbility.h"

#include "AbilitySystem/AgentAbilitySystemComponent.h"

bool UBaseGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		if (FollowUpInputTags.IsEmpty() == false)
		{
			asc->ResisterFollowUpInput(FollowUpInputTags);
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo,true,false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GA, asc가 AgentAbilitySystemComponent를 상속받지 않았어요."));
	}
	
}

void UBaseGameplayAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	UE_LOG(LogTemp, Warning, TEXT("스킬 InputPressed"));

	//TODO: (필요하다면) 후속 입력 키에 따른 분기 작성
	if (CurrentFollowUpInputTag.IsValid())
	{
		if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick)
		{
			UE_LOG(LogTemp, Warning, TEXT("스킬 분기, 좌클릭"));
			return;
		}
		if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Default_RightClick)
		{
			UE_LOG(LogTemp, Warning, TEXT("스킬 분기, 우클릭"));
			return;
		}
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo,true,false);
}

void UBaseGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	UE_LOG(LogTemp, Warning, TEXT("스킬 InputReleased"));
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	UE_LOG(LogTemp, Warning, TEXT("스킬 EndAbility"));
	
	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		asc->ClearCurrentAbilityHandle(Handle);
	}
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
	UE_LOG(LogTemp, Warning, TEXT("스킬 CancelAbility"));

	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		asc->ClearCurrentAbilityHandle(Handle);
	}
}
