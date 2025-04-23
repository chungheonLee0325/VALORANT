// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"

#include "GameplayTagsManager.h"
#include "Abilities/BaseGameplayAbility.h"
#include "Attributes/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/Player/Agent/BaseAgent.h"


class UBaseGameplayAbility;

UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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
	
	DOREPLIFETIME(UAgentAbilitySystemComponent, AgentName);
	DOREPLIFETIME(UAgentAbilitySystemComponent, SkillQName);
	DOREPLIFETIME(UAgentAbilitySystemComponent, SkillCName);
	DOREPLIFETIME(UAgentAbilitySystemComponent, SkillEName);
	DOREPLIFETIME(UAgentAbilitySystemComponent, SkillXName);
	DOREPLIFETIME(UAgentAbilitySystemComponent, m_AgentID);
	// DOREPLIFETIME(UAgentAbilitySystemComponent, AgentSkillHandle);
}

/**서버에서만 호출됩니다.*/
void UAgentAbilitySystemComponent::InitializeByAgentData(int32 agentID)
{
	UValorantGameInstance* gi = Cast<UValorantGameInstance>(GetWorld()->GetGameInstance());
	FAgentData* data = gi->GetAgentData(agentID);

	AgentName = data->AgentName;
	
	InitializeAttribute(data);
	RegisterAgentAbilities(data);
}

void UAgentAbilitySystemComponent::InitializeAttribute(const FAgentData* agentData)
{
	SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), agentData->BaseHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), agentData->MaxHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(),agentData->BaseArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), agentData->MaxArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), agentData->Speed);

	// UE_LOG(LogTemp, Warning, TEXT("Health: %f"), GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute()));
	// UE_LOG(LogTemp, Warning, TEXT("Armor: %f"), GetNumericAttribute(UBaseAttributeSet::GetArmorAttribute()));
	// UE_LOG(LogTemp, Warning, TEXT("WalkSpeed: %f"), GetNumericAttribute(UBaseAttributeSet::GetMoveSpeedAttribute()));
}

void UAgentAbilitySystemComponent::RegisterAgentAbilities(const FAgentData* agentData)
{
	SetAgentAbility(agentData->Ability_Q, 1);
	SetAgentAbility(agentData->Ability_C, 1);
	SetAgentAbility(agentData->Ability_E, 1);
	SetAgentAbility(agentData->Ability_X, 1);
}

//TODO: 인자에 const FGameplayTag& tag 추가를 고려해볼 것.
void UAgentAbilitySystemComponent::SetAgentAbility(TSubclassOf<UGameplayAbility> abilityClass, int32 level)
{
	UGameplayAbility* ga = abilityClass->GetDefaultObject<UGameplayAbility>();
	FGameplayAbilitySpec spec(abilityClass, level);
	
	bool bIsSkill = false;
	FGameplayTag skillTag;
	
	const FGameplayTagContainer& tagCon = ga->GetAssetTags();

	if (tagCon.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Ability [%s]에 태그가 없습니다."), *GetNameSafe(ga));
		return;
	}

	for (FGameplayTag tag: tagCon)
	{
		if (SkillTags.Contains(tag))
		{
			skillTag = tag;		
			bIsSkill = true;
			break;
		}
	}

	if (bIsSkill == false)
	{
		UE_LOG(LogTemp, Error, TEXT("%s는 Input.Skill.~ 를 지닌 어빌리티가 아니므로, 등록할 수 없습니다."), *this->GetName());
		return;
	}

	FGameplayAbilitySpecHandle* existHandle = ReservedSkillHandleMap.Find(skillTag);
	if (existHandle)
	{
		ClearAbility(*existHandle);
		ReservedSkillHandleMap.Remove(skillTag);
		UE_LOG(LogTemp,Warning,TEXT("%s 스킬이 이미 존재해요. 제거 후 재등록합니다."),*skillTag.ToString());
	}

	//현재는 Tmap을 기반으로 스킬을 탐색하기 때문에, 당장 사용되지는 않습니다.
	spec.GetDynamicSpecSourceTags().AddTag(skillTag);
	
	//GiveAbility 다음 tick에서 spec의 handle이 생성되므로, handle을 리턴값으로 받아줘야 정확함.
	FGameplayAbilitySpecHandle handle = GiveAbility(spec);
	ReservedSkillHandleMap.Add(skillTag, handle);

	// UE_LOG(LogTemp,Warning,TEXT("%s 스킬 등록."),*skillTag.ToString());
}

void UAgentAbilitySystemComponent::ResetAgentAbilities()
{
	UE_LOG(LogTemp,Warning,TEXT("모든 스킬 리셋"));
	
	TArray<FGameplayTag> tagsToRemove;
	ReservedSkillHandleMap.GetKeys(tagsToRemove);

	for (const FGameplayTag& tag : tagsToRemove)
	{
		ClearAbility(ReservedSkillHandleMap[tag]);
		ReservedSkillHandleMap.Remove(tag);
	}
}

void UAgentAbilitySystemComponent::ResisterFollowUpInput(const TSet<FGameplayTag>& tags)
{
	FollowUpInputBySkill = tags;
	for (FGameplayTag tag: tags)
	{
		UE_LOG(LogTemp,Warning,TEXT("%s 후속 입력 키로 등록"), *tag.GetTagName().ToString());
		OnAbilityWaitingStateChanged.Broadcast(true);
	}
}

bool UAgentAbilitySystemComponent::TrySkillInput(const FGameplayTag& inputTag)
{
	if (FollowUpInputBySkill.IsEmpty())
	{
		if (const FGameplayAbilitySpecHandle* gaHandle = ReservedSkillHandleMap.Find(inputTag))
		{
			if (TryActivateAbility(*gaHandle))
			{
				//UE_LOG(LogTemp,Warning,TEXT("스킬 일반 입력 성공"));
				CurrentAbilityHandle = *gaHandle;
				return true;
			}
			else
			{
				UE_LOG(LogTemp,Warning,TEXT("스킬 일반 입력 실패"));
				return false;
			}
		}
	}
	else
	{
		if (IsFollowUpInput(inputTag))
		{
			if (TrySkillFollowupInput(inputTag))
			{
				//UE_LOG(LogTemp,Warning,TEXT("스킬 후속 입력 성공"));
				FollowUpInputBySkill.Empty();
				OnAbilityWaitingStateChanged.Broadcast(false);
				return true;
			}
			else
			{
				UE_LOG(LogTemp,Warning,TEXT("스킬 후속 입력 실패"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("후속 입력 대기 중이라 일반 입력 [%s] 무시됨"), *inputTag.ToString());
			return false;
		}
	}
	
	// UE_LOG(LogTemp,Warning,TEXT("%s 해당 입력에 해당하는 스킬이 없습니다."), *inputTag.ToString());
	return false;
}

//TODO: 추후 NotifyEnd로 옮기기
void UAgentAbilitySystemComponent::ClearCurrentAbilityHandle(const FGameplayAbilitySpecHandle handle)
{
	if (!CurrentAbilityHandle.IsValid())
	{
		//UE_LOG(LogTemp, Warning, TEXT("CurrentAbilityHandle은 이미 무효상태입니다."));
		return;
	}
	if (!handle.IsValid())
	{
		//UE_LOG(LogTemp, Warning, TEXT("handle 이미 무효상태입니다."));
		return;
	}
	
	if (CurrentAbilityHandle.IsValid() && CurrentAbilityHandle == handle)
	{
		CurrentAbilityHandle = FGameplayAbilitySpecHandle();
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("실제 사용 완료된 Ability와 ASC의 CurrentAbility 변수의 정보가 일치하지 않아요."));
	}
}

bool UAgentAbilitySystemComponent::IsFollowUpInput(const FGameplayTag& inputTag)
{
	return (FollowUpInputBySkill.Contains(inputTag));
}

bool UAgentAbilitySystemComponent::TrySkillFollowupInput(const FGameplayTag& inputTag)
{
	if (CurrentAbilityHandle.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("후속 입력 대기 중인 스킬이 없습니다."));
		return false;
	}
	
	FGameplayAbilitySpec* spec = FindAbilitySpecFromHandle(CurrentAbilityHandle);
	if (spec && spec->IsActive())
	{
		if (UBaseGameplayAbility* ga = Cast<UBaseGameplayAbility>(spec->Ability))
		{
			ga->CurrentFollowUpInputTag = inputTag;
		}
		
		AbilitySpecInputPressed(*spec);
		return true;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ASC내에서 후속 입력 대기 중인 스킬과 실제 실행 중인 스킬이 불일치합니다."));
	return false;
}

