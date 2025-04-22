// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BaseGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

	//TODO: CanActivateAbility(): 스킬 실행 가능한지 체크, 쿨타임 블락태그 등
	//TODO: ActiveAbility(): 스킬 가짜 실행, (필요하다면) ASC에 후속 입력 키 등록
	//TODO: InputPressd(): 스킬 진짜 실행
	//TODO: InputReleased()
	//TODO: EndAbility(): ASC->ClearCurrnetAbility 호출
	//TODO: CancelAbility(): ASC->ClearCurrnetAbility 호출

	// Ability에서 ASC가져오는 법?
	// UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	// 직접 멤버 변수로 캐싱해도 되지만, 필요할 때마다 호출해도 퍼포먼스 이슈 없음
};
