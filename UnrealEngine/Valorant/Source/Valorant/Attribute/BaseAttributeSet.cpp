// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
#include "Valorant/Agent/BaseAgent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UBaseAttributeSet::UBaseAttributeSet()
{
}

// 속성에 변경된 값을 적용하기 직전 호출되는 함수. 최종적으로 값을 점검하기 위한 용도 (e.g. 최대값을 넘지 않도록 제한 / 비율 유지)
// SetCurrentValue 처럼 Gas나 GE에 의해 변경되지 않는 경우에는 호출되지 않는다
void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

// 속성이 바뀐 직후 호출되는 함수.
// 이펙트, 애니메이션 등의 트리거 / 사망 처리 / 태그 조건에 따른 추가 처리
void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UBaseAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute,
                                                    const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
}

void UBaseAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
}

void UBaseAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
}