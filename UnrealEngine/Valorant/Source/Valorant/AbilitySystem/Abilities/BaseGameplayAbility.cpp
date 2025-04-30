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
		asc->SetCurrentAbilityHandle(Handle);
		
		if (FollowUpInputTags.IsEmpty() == false)
		{
			asc->ResisterFollowUpInput(FollowUpInputTags);
		}
		else
		{
			Active_General();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
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
	
	if (CurrentFollowUpInputTag.IsValid())
	{
		if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick)
		{
			Active_Left_Click();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		}
		else if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Default_RightClick)
		{
			Active_Right_Click();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		}
		else if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Ability_C)
		{
			Active_C_Click();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		}
		else if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Ability_E)
		{
			Active_E_Click();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		}
		else if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Ability_Q)
		{
			Active_Q_Click();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		}
		else if (CurrentFollowUpInputTag == FValorantGameplayTags::Get().InputTag_Ability_X)
		{
			Active_X_Click();
			EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		}
	}
	
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

	CurrentFollowUpInputTag = FGameplayTag(); 
	
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

	CurrentFollowUpInputTag = FGameplayTag();
	
	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		asc->ClearCurrentAbilityHandle(Handle);
	}
}

void UBaseGameplayAbility::Active_General()
{
}

void UBaseGameplayAbility::Active_Left_Click()
{
}

void UBaseGameplayAbility::Active_Right_Click()
{
}

void UBaseGameplayAbility::Active_C_Click()
{
}

void UBaseGameplayAbility::Active_E_Click()
{
}

void UBaseGameplayAbility::Active_Q_Click()
{
}

void UBaseGameplayAbility::Active_X_Click()
{
}

void UBaseGameplayAbility::SetAbilityID(int32 AbilityID)
{
	m_AbilityID = AbilityID;
}
