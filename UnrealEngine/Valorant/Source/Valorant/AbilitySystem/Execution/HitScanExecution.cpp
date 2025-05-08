// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanExecution.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "AbilitySystem/Context/HitScanGameplayEffectContext.h"

void UHitScanExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                               FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	int FinalDamage = 0;
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FHitScanGameplayEffectContext* HitScanContext = static_cast<const FHitScanGameplayEffectContext*>(Spec.GetContext().Get());
	if (HitScanContext)
	{
		FinalDamage = HitScanContext->Damage;
		// TODO: 상대 공격력감소버프나 자기자신의 피격뎀감/뎀증 버프디버프 있으면 여기서 추가연산 하면 될듯
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%hs Called, FinalDamage: %d"), __FUNCTION__, FinalDamage);
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBaseAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));
}
