// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"

#include "GameplayTagsManager.h"
#include "Attributes/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/Player/Agent/BaseAgent.h"


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
	DOREPLIFETIME(UAgentAbilitySystemComponent, AgentSkillHandle);
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

void UAgentAbilitySystemComponent::SkillCallByTag(const FGameplayTag& inputTag)
{
	FGameplayTagContainer tagCon(inputTag);
	bool bSuccess = TryActivateAbilitiesByTag(tagCon);
	
	UE_LOG(LogTemp, Warning, TEXT("GA TryActivate by Tag: %s → %s"), *inputTag.ToString(), bSuccess ? TEXT("성공") : TEXT("실패"));
}

void UAgentAbilitySystemComponent::InitializeAttribute(const FAgentData* agentData)
{
	SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), agentData->BaseHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), agentData->MaxHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(),agentData->BaseArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), agentData->MaxArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), agentData->Speed);

	UE_LOG(LogTemp, Warning, TEXT("Health: %f"), GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute()));
	UE_LOG(LogTemp, Warning, TEXT("Armor: %f"), GetNumericAttribute(UBaseAttributeSet::GetArmorAttribute()));
	UE_LOG(LogTemp, Warning, TEXT("WalkSpeed: %f"), GetNumericAttribute(UBaseAttributeSet::GetMoveSpeedAttribute()));
}

void UAgentAbilitySystemComponent::RegisterAgentAbilities(const FAgentData* agentData)
{
	GiveAgentAbility(agentData->Ability_Q, 1);
	GiveAgentAbility(agentData->Ability_C, 1);
	GiveAgentAbility(agentData->Ability_E, 1);
	GiveAgentAbility(agentData->Ability_X, 1);
}

void UAgentAbilitySystemComponent::ClearAgentAbilities()
{
	FGameplayTagContainer tagCon;
	for (FGameplayTag tag : SkillTags)
	{
		tagCon.AddTag(tag);
	}
	ClearAgentAbility(tagCon);
}

//TODO: 인자에 const FGameplayTag& tag 추가를 고려해볼 것.
void UAgentAbilitySystemComponent::GiveAgentAbility(TSubclassOf<UGameplayAbility> abilityClass, int32 level)
{
	UGameplayAbility* ga = abilityClass->GetDefaultObject<UGameplayAbility>();
	FGameplayAbilitySpec spec(abilityClass, level);
	//TODO: 스킬에 태그 늘어나면 변경
	FGameplayTag skillTag = ga->AbilityTags.First();

	if (!SkillTags.Contains(skillTag))
	{
		UE_LOG(LogTemp, Error, TEXT("AbilityTypeTag %s는 스킬용 태그가 아니므로 등록할 수 없습니다."), *skillTag.ToString());
		return;
	}

	//TODO: 레거시. 프로토 UI에 스킬 네임 전달하기 위함
	if (skillTag == FValorantGameplayTags::Get().InputTag_Ability_Q)
	{
		 SkillQName = abilityClass->GetName();
	}
	else if (skillTag == FValorantGameplayTags::Get().InputTag_Ability_C)
	{
		SkillCName = abilityClass->GetName();
	}
	else if (skillTag == FValorantGameplayTags::Get().InputTag_Ability_E)
	{
		SkillEName = abilityClass->GetName();
	}
	else if (skillTag == FValorantGameplayTags::Get().InputTag_Ability_X)
	{
		SkillXName = abilityClass->GetName();
	}
		
	//GiveAbility 다음 tick에서 spec의 handle이 생성되므로, handle을 리턴값으로 받아줘야 정확함.
	spec.GetDynamicSpecSourceTags().AddTag(ga->AbilityTags.First());
	FGameplayAbilitySpecHandle handle = GiveAbility(spec);
	AgentSkillHandle.Add(handle);
}

void UAgentAbilitySystemComponent::ClearAgentAbility(const FGameplayTagContainer& tags)
{
	for (const FGameplayAbilitySpec& spec : GetActivatableAbilities())
	{
		if (spec.GetDynamicSpecSourceTags().HasAny(tags))
		{
			ClearAbility(spec.Handle);
		}
	}
}

// void UAgentAbilitySystemComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
// {
// }
//
// void UAgentAbilitySystemComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
// {
// }
//
// void UAgentAbilitySystemComponent::HandleArmorChanged(const FOnAttributeChangeData& Data)
// {
// }
//
// void UAgentAbilitySystemComponent::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
// {
// }
