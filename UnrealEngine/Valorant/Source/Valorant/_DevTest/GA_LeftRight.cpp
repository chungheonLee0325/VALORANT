// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_LeftRight.h"

#include "EnhancedInputComponent.h"
#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "GameManager/SubsystemSteamManager.h"


void UGA_LeftRight::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// UE_LOG(LogTemp,Warning,TEXT("준비 동작 실행"));


	if (HasAuthority(&ActivationInfo))
	{
		MontageStop();
		// 대기 동작을 위한 MontageTask
		UAbilityTask_PlayMontageAndWait* PreTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,ReadyAnim,1.f,NAME_None,true);
	
		PreTask->OnCompleted.AddDynamic(this,&UGA_LeftRight::OnPreMontageFinished);
		PreTask->OnCancelled.AddDynamic(this,&UGA_LeftRight::OnPreMontageCancelled);
	
		PreTask->ReadyForActivation();
	}
}

void UGA_LeftRight::OnPreMontageFinished()
{
	// UE_LOG(LogTemp,Warning,TEXT("준비 동작 종료"));
	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		//asc->SetSkillReady(true);
	}

	// 실질 실행을 위한 후속 입력 대기 Task
	FGameplayTag LeftTag = FValorantGameplayTags::Get().InputTag_Default_LeftClick;
	
	UAbilityTask_WaitGameplayEvent* LeftTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, LeftTag, nullptr, true, true);
	LeftTask->EventReceived.AddDynamic(this, &UGA_LeftRight::Active_Left_Click);
	LeftTask->Activate();
	
	FGameplayTag RightTag = FValorantGameplayTags::Get().InputTag_Default_RightClick;
	
	UAbilityTask_WaitGameplayEvent* RightTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, RightTag, nullptr, true, true);
	RightTask->EventReceived.AddDynamic(this, &UGA_LeftRight::Active_Right_Click);
	RightTask->Activate();
}

void UGA_LeftRight::OnPreMontageCancelled()
{
	// UE_LOG(LogTemp,Log,TEXT("준비 동작 끊김"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_LeftRight::Active_Left_Click(FGameplayEventData data)
{
	UE_LOG(LogTemp,Log,TEXT("Task에 의한 좌클릭 후속 입력"));
	
	UAbilityTask_PlayMontageAndWait* ThrowTask =
	  UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, LeftAnim, 1.f, NAME_None, false);

	MainTask(ThrowTask);
}

void UGA_LeftRight::Active_Right_Click(FGameplayEventData data)
{
	UE_LOG(LogTemp,Log,TEXT("Task에 의한 우클릭 후속 입력"));

	UAbilityTask_PlayMontageAndWait* ThrowTask =
	  UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, RightAnim, 1.f, NAME_None, false);
	
	MainTask(ThrowTask);
}

void UGA_LeftRight::MainTask(UAbilityTask_PlayMontageAndWait* ThrowTask)
{
	ThrowTask->OnCompleted.AddDynamic(this,&UGA_LeftRight::OnProcessMontageFinished);
	ThrowTask->OnCancelled.AddDynamic(this,&UGA_LeftRight::OnProcessMontageCancelled);

	ThrowTask->ReadyForActivation();
}

void UGA_LeftRight::OnProcessMontageFinished()
{
	// UE_LOG(LogTemp,Warning,TEXT("스킬 동작 종료"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_LeftRight::OnProcessMontageCancelled()
{
	UE_LOG(LogTemp,Warning,TEXT("스킬 동작 끊김"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
