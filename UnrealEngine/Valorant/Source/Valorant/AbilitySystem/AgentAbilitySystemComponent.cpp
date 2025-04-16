// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"

#include "GameplayTagsManager.h"
#include "Abilities/GameplayAbilityWithTag.h"
#include "Attributes/BaseAttributeSet.h"


UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAgentAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	auto& tagManager = UGameplayTagsManager::Get();
	InputSlots.Add(tagManager.RequestGameplayTag("Input.Skill.Q"));
	InputSlots.Add(tagManager.RequestGameplayTag("Input.Skill.E"));
	InputSlots.Add(tagManager.RequestGameplayTag("Input.Skill.X"));
	InputSlots.Add(tagManager.RequestGameplayTag("Input.Skill.C"));
}

void UAgentAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAgentAbilitySystemComponent::InitializeAgentData(FAgentData* agentData)
{
	m_AgentData = agentData;
	
	InitializeAttribute();
	RegisterAgentAbilities();
}

void UAgentAbilitySystemComponent::SkillCallByTag(const FGameplayTag& inputTag)
{
	if (const FGameplayAbilitySpecHandle* gaHandle = SkillHandleMap.Find(inputTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 호출"));
		TryActivateAbility(*gaHandle);
	}
}

void UAgentAbilitySystemComponent::InitializeAttribute()
{
	SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), m_AgentData->BaseHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), m_AgentData->MaxHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(),m_AgentData->BaseArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), m_AgentData->MaxArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), m_AgentData->Speed);
}

void UAgentAbilitySystemComponent::RegisterAgentAbilities()
{
	GiveAgentAbility(m_AgentData->Ability_Q, 1);
	GiveAgentAbility(m_AgentData->Ability_C, 1);
	GiveAgentAbility(m_AgentData->Ability_E, 1);
	GiveAgentAbility(m_AgentData->Ability_X, 1);
}

void UAgentAbilitySystemComponent::ClearAgentAbilities()
{
	for (FGameplayTag inputSlot : InputSlots)
	{
		FGameplayAbilitySpecHandle* handle = SkillHandleMap.Find(inputSlot);
		if (handle)
		{
			ClearAgentAbility(FindAbilitySpecFromHandle(*handle));
		}
	}
	
}

//TODO: 인자에 const FGameplayTag& tag 추가를 고려해볼 것.
void UAgentAbilitySystemComponent::GiveAgentAbility(TSubclassOf<UGameplayAbility> abilityClass, int32 level)
{
	UGameplayAbility* cdo = abilityClass->GetDefaultObject<UGameplayAbility>();
	FGameplayAbilitySpec spec(abilityClass, level);
	
	const UGameplayAbilityWithTag* ga = Cast<UGameplayAbilityWithTag>(cdo);
	if (ga ==nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("등록하고자 하는 스킬이 UGameplayAbilityWithTag 가 아니에요."));
		return;
	}

	if (!InputSlots.Contains(ga->AbilityTypeTag))
	{
		UE_LOG(LogTemp, Error, TEXT("AbilityTypeTag %s는 스킬용 태그가 아니므로 등록할 수 없습니다."), *ga->AbilityTypeTag.ToString());
		return;
	}

	FGameplayAbilitySpecHandle* existHandle = SkillHandleMap.Find(ga->AbilityTypeTag);
	if (existHandle)
	{
		ClearAbility(*existHandle);
		SkillHandleMap.Remove(ga->AbilityTypeTag);
		UE_LOG(LogTemp,Warning,TEXT("%s 스킬이 이미 존재해요. 제거 후 재등록합니다."),*ga->AbilityTypeTag.ToString());
	}

	spec.GetDynamicSpecSourceTags().AddTag(ga->AbilityTypeTag);

	//GiveAbility 다음 tick에서 spec의 handle이 생성되므로, handle을 리턴값으로 받아줘야 정확함.
	FGameplayAbilitySpecHandle handle = GiveAbility(spec);
	SkillHandleMap.Add(ga->AbilityTypeTag, handle);
}

void UAgentAbilitySystemComponent::ClearAgentAbility(FGameplayAbilitySpec* spec)
{
	if (spec == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("SKill Spec is NULL"));
		return;
	}
	const UGameplayAbilityWithTag* ga = Cast<UGameplayAbilityWithTag>(spec->Ability);

	for (FGameplayTag inputSlot : InputSlots)
	{
		if (inputSlot == ga->AbilityTypeTag)
		{
			SkillHandleMap.Remove(ga->AbilityTypeTag);
			
			ClearAbility(spec->Handle);
			break;
		}
	}
}