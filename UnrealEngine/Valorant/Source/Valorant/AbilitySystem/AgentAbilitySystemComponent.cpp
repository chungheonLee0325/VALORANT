// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"

#include <GameManager/SubsystemSteamManager.h>

#include "GameplayTagsManager.h"
#include "Valorant.h"
#include "Abilities/BaseGameplayAbility.h"
#include "Attributes/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/Player/Agent/BaseAgent.h"


class UBaseGameplayAbility;

UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

void UAgentAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	auto& tagManager = UGameplayTagsManager::Get();
	SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.Q"));
	SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.E"));
	SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.X"));
	SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.C"));
	
}

void UAgentAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAgentAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UAgentAbilitySystemComponent, m_AgentID);
	DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_C);
	DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_E);
	DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_Q);
	DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_X);
	// DOREPLIFETIME(UAgentAbilitySystemComponent, CurrentAbilityHandle);
}

int32 UAgentAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("핸들 게임플레이 이벤트"));
	return Super::HandleGameplayEvent(EventTag, Payload);
}

/**서버에서만 호출됩니다.*/
void UAgentAbilitySystemComponent::InitializeByAgentData(int32 agentID)
{
	m_GameInstance = Cast<UValorantGameInstance>(GetWorld()->GetGameInstance());
	FAgentData* data = m_GameInstance->GetAgentData(agentID);
	
	InitializeAttribute(data);
	RegisterAgentAbilities(data);
}

void UAgentAbilitySystemComponent::InitializeAttribute(const FAgentData* agentData)
{
	SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), agentData->BaseHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), agentData->MaxHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(),agentData->BaseArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), agentData->MaxArmor);

	// UE_LOG(LogTemp, Warning, TEXT("Health: %f"), GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute()));
	// UE_LOG(LogTemp, Warning, TEXT("Armor: %f"), GetNumericAttribute(UBaseAttributeSet::GetArmorAttribute()));
	// UE_LOG(LogTemp, Warning, TEXT("WalkSpeed: %f"), GetNumericAttribute(UBaseAttributeSet::GetMoveSpeedAttribute()));
}

void UAgentAbilitySystemComponent::RegisterAgentAbilities(const FAgentData* agentData)
{
	NET_LOG(LogTemp,Warning,TEXT("Ability ID : %d(C), %d(E), %d(Q), %d(X)"), agentData->AbilityID_C, agentData->AbilityID_E,agentData->AbilityID_Q, agentData->AbilityID_X);
	SetAgentAbility(agentData->AbilityID_C, 1);
	SetAgentAbility(agentData->AbilityID_E, 1);
	SetAgentAbility(agentData->AbilityID_Q, 1);
	SetAgentAbility(agentData->AbilityID_X, 1);
}

void UAgentAbilitySystemComponent::SetAgentAbility(int32 abilityID, int32 level)
{
	FAbilityData* abilityData = m_GameInstance->GetAbilityData(abilityID);
	TSubclassOf<UGameplayAbility> abilityClass = abilityData->AbilityClass;
	// NET_LOG(LogTemp,Warning,TEXT("ability Name: %s"),*abilityClass->GetName());
	
	UGameplayAbility* ga = abilityClass->GetDefaultObject<UGameplayAbility>();
	const FGameplayTagContainer& tagCon = ga->GetAssetTags();

	if (tagCon.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Ability [%s]에 태그가 없습니다."), *GetNameSafe(ga));
		return;
	}

	bool bIsSkill = false;
	FGameplayTag skillTag;

	for (const FGameplayTag& tag : tagCon)
	{
		// NET_LOG(LogTemp,Warning,TEXT("tag: %s"), *tag.GetTagName().ToString());
		const FGameplayTag* foundTag = SkillTags.Find(tag);
		if (foundTag)
		{
			if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_C)
			{
				m_Ability_C = *abilityData;
			}
			else if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_E)
			{
				m_Ability_E = *abilityData;
			}
			else if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_Q)
			{
				m_Ability_Q = *abilityData;
			}
			else if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_X)
			{
				m_Ability_X = *abilityData;
			}
			bIsSkill = true;
		}
	}

	if (bIsSkill == false)
	{
		UE_LOG(LogTemp, Error, TEXT("%s는 Input.Skill.~ 태그를 지닌 어빌리티가 아니므로, 등록할 수 없습니다."), *abilityClass->GetName());
		return;
	}

	// FGameplayAbilitySpecHandle* existHandle = ReservedSkillHandleMap.Find(skillTag);
	// if (existHandle)
	// {
	// 	ClearAbility(*existHandle);
	// 	ReservedSkillHandleMap.Remove(skillTag);
	// 	UE_LOG(LogTemp,Warning,TEXT("%s 스킬이 이미 존재해요. 제거 후 재등록합니다."),*skillTag.ToString());
	// }
	
	FGameplayAbilitySpec spec(abilityClass, level);
	spec.GetDynamicSpecSourceTags().AddTag(skillTag);
	GiveAbility(spec);

	// UE_LOG(LogTemp,Warning,TEXT("%s 스킬 등록."),*skillTag.ToString());
}

void UAgentAbilitySystemComponent::ResetAgentAbilities()
{
	UE_LOG(LogTemp,Warning,TEXT("모든 스킬 리셋"));
	
	for (const FGameplayAbilitySpec& spec: GetActivatableAbilities())
	{
		ClearAbility(spec.Handle);
	}
}

void UAgentAbilitySystemComponent::ResisterFollowUpInput(const TSet<FGameplayTag>& tags)
{
	FollowUpInputBySkill = tags;
	for (FGameplayTag tag: tags)
	{
		UE_LOG(LogTemp,Warning,TEXT("%s 후속 입력 키로 등록"), *tag.GetTagName().ToString());
	}
	OnAbilityWaitingStateChanged.Broadcast(true);
}

void UAgentAbilitySystemComponent::ResetFollowUpInput()
{
	FollowUpInputBySkill.Empty();
}

bool UAgentAbilitySystemComponent::TrySkillInput(const FGameplayTag& inputTag)
{
	if (FollowUpInputBySkill.IsEmpty())
	{
		UE_LOG(LogTemp,Warning,TEXT("일반 입력 시도: [%s]"), *inputTag.ToString());
		if (!bIsSkillClear)
		{
			// UE_LOG(LogTemp,Error,TEXT("이전 스킬 마무리중..."));
			return true;
		}
		
		FGameplayTagContainer tagCon(inputTag);
		if (TryActivateAbilitiesByTag(tagCon))
		{
			UE_LOG(LogTemp,Warning,TEXT("스킬 일반 입력 성공"));
			return true;
		}
	}
	else
	{ 
		UE_LOG(LogTemp,Warning,TEXT("스킬 후속 입력 시도: [%s]"), *inputTag.ToString());
		
		if (!bIsSkillReady)
		{
			// UE_LOG(LogTemp,Error,TEXT("준비 동작 진행중..."));
			return true;
		}
		
		if (IsFollowUpInput(inputTag))
		{
			UE_LOG(LogTemp,Warning,TEXT("스킬 후속 입력 성공"));

			FGameplayEventData data;
			data.EventTag = inputTag;
			
			HandleGameplayEvent(inputTag, &data);
			
			FollowUpInputBySkill.Empty();
			OnAbilityWaitingStateChanged.Broadcast(false);
		}
		else
		{
			NET_LOG(LogTemp, Warning, TEXT("후속 입력 대기 중이라 일반 입력 [%s] 무시됨"), *inputTag.ToString());
		}
		return true;
	}
	
	// UE_LOG(LogTemp,Warning,TEXT("%s 해당 입력에 해당하는 스킬이 없습니다."), *inputTag.ToString());
	return false;
}

void UAgentAbilitySystemComponent::Net_ReserveSkill_Implementation(const FGameplayTag& skillTag,
	const FGameplayAbilitySpecHandle& handle)
{
}

void UAgentAbilitySystemComponent::Net_ResetSkill_Implementation(const TArray<FGameplayTag>& tagsToRemove)
{
}

bool UAgentAbilitySystemComponent::IsFollowUpInput(const FGameplayTag& inputTag)
{
	return (FollowUpInputBySkill.Contains(inputTag));
}
