// Fill out your copyright notice in the Description page of Project Settings.


#include "EffectManager.h"

#include "Valorant/AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Valorant/GameManager/ValorantGameInstance.h"

void UEffectManager::InitManager(UValorantGameInstance* GameInstance)
{
	m_GameInstance = GameInstance;
}

void UEffectManager::ApplyEffectToTarget(AActor* Target, int EffectID, AActor* Instigator)
{
	if (!Target) return;
    
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC) return;
    
	FGameplayEffectData* EffectData = m_GameInstance->GetGEffectData(EffectID);
	if (!EffectData || !EffectData->EffectClass) return;
    
	// 효과 컨텍스트 생성
	FGameplayEffectContextHandle EffectContext;
	if (Instigator)
	{
		UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
		if (InstigatorASC)
		{
			EffectContext = InstigatorASC->MakeEffectContext();
			EffectContext.AddInstigator(Instigator, Instigator);
		}
	}
	else
	{
		EffectContext = TargetASC->MakeEffectContext();
	}
    
	// 효과 스펙 생성
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectData->EffectClass, 1, EffectContext);
	if (SpecHandle.IsValid())
	{
		// 기본 크기 설정
		if (EffectData->Magnitude != 0)
		{
			SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Magnitude")), EffectData->Magnitude);
		}
        
		// 지속시간 설정
		if (EffectData->Duration > 0)
		{
			SpecHandle.Data->SetDuration(EffectData->Duration, true);
		}
        
		// SetByCaller 값 설정
		for (auto& Pair : EffectData->SetByCallerMagnitudes)
		{
			SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
		}
        
		// 효과 적용
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
