// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystem/Tasks/PlayMontageWithEvent.h"
#include "GA_LeftRight.generated.h"

/**
 * 피닉스 커브볼 스킬을 구현
 * 즉발성이 아닌 스킬은 GamePlayTask를 2개 사용합니다.
 * 준비 동작 Task: UAbilityTask_PlayMontageAndWait,	커브볼을 손에 드는 기능
 * 실제 스킬 Task: UAbilityTask_WaitGameplayEvent,	커브볼을 손에 들고 좌/우 클릭을 대기하는 기능
 */

UCLASS()
class VALORANT_API UGA_LeftRight : public UBaseGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	UAnimMontage* ReadyAnim = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	UAnimMontage* LeftAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	UAnimMontage* RightAnim = nullptr;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnPreMontageFinished();
	UFUNCTION()
	void OnPreMontageCancelled();
	UFUNCTION()
	void OnProcessMontageFinished();
	UFUNCTION()
	void OnProcessMontageCancelled();

	virtual void Active_Left_Click(FGameplayEventData data);
	virtual void Active_Right_Click(FGameplayEventData data);
	
	void MainTask(UAbilityTask_PlayMontageAndWait* ThrowTask);

};
