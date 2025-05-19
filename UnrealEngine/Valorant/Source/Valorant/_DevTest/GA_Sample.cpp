// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Sample.h"

#include "EnhancedInputComponent.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"


void UGA_Sample::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// UE_LOG(LogTemp,Warning,TEXT("준비 동작 실행"));

	// 대기 동작을 위한 MontageTask
	UAbilityTask_PlayMontageAndWait* PreTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,ReadyAnim,1.f,NAME_None,true);
	
	PreTask->OnCompleted.AddDynamic(this,&UGA_Sample::OnPreMontageFinished);
	PreTask->OnCancelled.AddDynamic(this,&UGA_Sample::OnPreMontageCancelled);
	
	PreTask->ReadyForActivation();
}

void UGA_Sample::OnPreMontageFinished()
{
	// UE_LOG(LogTemp,Warning,TEXT("준비 동작 종료"));
	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		asc->SetSkillReady(true);
	}

	// 실질 실행을 위한 후속 입력 대기 Task
	FGameplayTag LeftTag = FValorantGameplayTags::Get().InputTag_Default_LeftClick;
	LeftTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, LeftTag, nullptr, true, true);
	LeftTask->EventReceived.AddDynamic(this, &UGA_Sample::Active_Left_Click);
	LeftTask->Activate();
	
	FGameplayTag RightTag = FValorantGameplayTags::Get().InputTag_Default_RightClick;
	RightTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, RightTag, nullptr, true, true);
	RightTask->EventReceived.AddDynamic(this, &UGA_Sample::Active_Right_Click);
	RightTask->Activate();
}

void UGA_Sample::OnPreMontageCancelled()
{
	// UE_LOG(LogTemp,Log,TEXT("준비 동작 끊김"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Sample::Active_Left_Click(FGameplayEventData data)
{
	UE_LOG(LogTemp,Log,TEXT("Task에 의한 좌클릭 후속 입력"));
	
	UAbilityTask_PlayMontageAndWait* ThrowTask =
	  UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, LeftAnim, 1.f, NAME_None, false);

	MainTask(ThrowTask);
}

void UGA_Sample::Active_Right_Click(FGameplayEventData data)
{
	UE_LOG(LogTemp,Log,TEXT("Task에 의한 우클릭 후속 입력"));

	UAbilityTask_PlayMontageAndWait* ThrowTask =
	  UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, RightAnim, 1.f, NAME_None, false);
	
	MainTask(ThrowTask);
}

void UGA_Sample::MainTask(UAbilityTask_PlayMontageAndWait* ThrowTask)
{
	// UE_LOG(LogTemp,Warning,TEXT("스킬 동작 실행"));
	LeftTask->EndTask();
	RightTask->EndTask();
	
	ThrowTask->OnCompleted.AddDynamic(this,&UGA_Sample::OnProcessMontageFinished);
	ThrowTask->OnCancelled.AddDynamic(this,&UGA_Sample::OnProcessMontageCancelled);

	ThrowTask->ReadyForActivation();
}

void UGA_Sample::OnProcessMontageFinished()
{
	// UE_LOG(LogTemp,Warning,TEXT("스킬 동작 종료"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Sample::OnProcessMontageCancelled()
{
	UE_LOG(LogTemp,Warning,TEXT("스킬 동작 끊김"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
