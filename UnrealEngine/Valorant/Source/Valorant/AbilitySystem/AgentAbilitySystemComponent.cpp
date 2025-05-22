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
}

int32 UAgentAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
    int32 Result = Super::HandleGameplayEvent(EventTag, Payload);
    
    // 현재 활성화된 어빌리티 찾기
    FGameplayAbilitySpec* FoundSpec = nullptr;
    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (Spec.IsActive())
        {
            FoundSpec = &Spec;
            break;
        }
    }

    if (!FoundSpec) return Result;

    UBaseGameplayAbility* AbilityInstance = Cast<UBaseGameplayAbility>(FoundSpec->GetPrimaryInstance());
    if (!AbilityInstance) return Result;

    FGameplayEventData EventData;
    if (Payload)
    {
        EventData = *Payload;
    }
    EventData.EventTag = EventTag;

    // 후속 입력인지 확인 후 적절한 메서드 호출
    if (IsWaitingForFollowUp() && AbilityInstance->IsValidFollowUpInput(EventTag))
    {
        // 후속 입력으로 처리
        AbilityInstance->HandleFollowUpInput(EventTag, EventData);
    }
    else if (EventTag == FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")))
    {
        // 일반 좌클릭 (후속 입력이 아닌 경우)
        AbilityInstance->HandleLeftClick(EventData);
    }
    else if (EventTag == FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")))
    {
        // 일반 우클릭 (후속 입력이 아닌 경우)
        AbilityInstance->HandleRightClick(EventData);
    }

    return Result;
}

void UAgentAbilitySystemComponent::ServerRPC_HandleGameplayEvent_Implementation(const FGameplayTag& inputTag)
{
    FGameplayEventData data;
    data.EventTag = inputTag;
    HandleGameplayEvent(inputTag, &data);
}

void UAgentAbilitySystemComponent::ServerRPC_SetAbilityState_Implementation(FGameplayTag StateTag, bool bApply)
{
    SetAbilityState(StateTag, bApply);
}

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
        const FGameplayTag* foundTag = SkillTags.Find(tag);
        if (foundTag)
        {
            if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_C)
            {
                m_Ability_C = *abilityData;
                AbilityDataMap.Add(*foundTag, *abilityData);
            }
            else if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_E)
            {
                m_Ability_E = *abilityData;
                AbilityDataMap.Add(*foundTag, *abilityData);
            }
            else if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_Q)
            {
                m_Ability_Q = *abilityData;
                AbilityDataMap.Add(*foundTag, *abilityData);
            }
            else if (*foundTag == FValorantGameplayTags::Get().InputTag_Ability_X)
            {
                m_Ability_X = *abilityData;
                AbilityDataMap.Add(*foundTag, *abilityData);
            }
            bIsSkill = true;
            skillTag = *foundTag;
        }
    }

    if (bIsSkill == false)
    {
        UE_LOG(LogTemp, Error, TEXT("%s는 Input.Skill.~ 태그를 지닌 어빌리티가 아니므로, 등록할 수 없습니다."), *abilityClass->GetName());
        return;
    }
    
    FGameplayAbilitySpec spec(abilityClass, level);
    spec.GetDynamicSpecSourceTags().AddTag(skillTag);
    GiveAbility(spec);
}

void UAgentAbilitySystemComponent::ResetAgentAbilities()
{
    UE_LOG(LogTemp,Warning,TEXT("모든 스킬 리셋"));
    
    for (const FGameplayAbilitySpec& spec: GetActivatableAbilities())
    {
        ClearAbility(spec.Handle);
    }
    
    AbilityDataMap.Empty();
}

// === 태그 기반 상태 관리 함수들 ===

bool UAgentAbilitySystemComponent::IsAbilityPreparing() const
{
    return HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
}

bool UAgentAbilitySystemComponent::IsAbilityExecuting() const
{
    return HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
}

bool UAgentAbilitySystemComponent::IsAbilityReady() const
{
    return HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Ready);
}

bool UAgentAbilitySystemComponent::IsWaitingForFollowUp() const
{
    return HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_WaitingFollowUp);
}

bool UAgentAbilitySystemComponent::CanActivateAbilities() const
{
    // 차단 태그가 없고, 실행 중이 아닐 때만 활성화 가능
    return !HasMatchingGameplayTag(FValorantGameplayTags::Get().Block_Ability_Activation) &&
           !IsAbilityExecuting();
}

void UAgentAbilitySystemComponent::SetAbilityState(FGameplayTag StateTag, bool bApply)
{
    if (bApply)
    {
        AddLooseGameplayTag(StateTag);
    }
    else
    {
        RemoveLooseGameplayTag(StateTag);
    }
    
    BroadcastStateChange(StateTag);
    
    // 서버에서 클라이언트로 동기화
    if (GetOwnerRole() == ROLE_Authority)
    {
        MulticastRPC_NotifyAbilityStateChanged(StateTag, bApply);
    }
}

void UAgentAbilitySystemComponent::RegisterFollowUpInputs(const TSet<FGameplayTag>& InputTags, FGameplayTag AbilityTag)
{
    CurrentFollowUpInputs = InputTags;
    CurrentExecutingAbility = AbilityTag;
    
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_WaitingFollowUp, true);
    
    for (FGameplayTag tag: InputTags)
    {
        UE_LOG(LogTemp,Warning,TEXT("%s 후속 입력 키로 등록"), *tag.GetTagName().ToString());
    }
    
    OnAbilityStateChanged.Broadcast(FValorantGameplayTags::Get().State_Ability_WaitingFollowUp);
}

void UAgentAbilitySystemComponent::ClearFollowUpInputs()
{
    CurrentFollowUpInputs.Empty();
    CurrentExecutingAbility = FGameplayTag();
    
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_WaitingFollowUp, false);
    
    OnAbilityStateChanged.Broadcast(FGameplayTag());
}

void UAgentAbilitySystemComponent::CleanupAbilityState()
{
    // 모든 어빌리티 상태 정리
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Preparing, false);
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Executing, false);
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Ready, false);
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_WaitingFollowUp, false);
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Charging, false);
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Aiming, false);
    SetAbilityState(FValorantGameplayTags::Get().State_Ability_Cooldown, false);
    
    ClearFollowUpInputs();
}

bool UAgentAbilitySystemComponent::TrySkillInput(const FGameplayTag& inputTag)
{
    if (!IsWaitingForFollowUp())
    {
        NET_LOG(LogTemp,Warning,TEXT("일반 입력 시도: [%s]"), *inputTag.ToString());
        
        if (IsAbilityExecuting())
        {
            // UE_LOG(LogTemp,Error,TEXT("이전 스킬 실행중..."));
            return true;
        }
        
        FGameplayTagContainer tagCon(inputTag);
        if (TryActivateAbilitiesByTag(tagCon))
        {
            NET_LOG(LogTemp,Warning,TEXT("스킬 일반 입력 성공"));
            return true;
        }
    }
    else
    { 
        NET_LOG(LogTemp,Warning,TEXT("스킬 후속 입력 시도: [%s]"), *inputTag.ToString());
        
        if (!IsAbilityReady())
        {
            // 준비 동작 진행중이거나 애니메이션 재생 중...
            return true;
        }
        
        if (IsValidFollowUpInput(inputTag))
        {
            NET_LOG(LogTemp,Warning,TEXT("스킬 후속 입력 성공"));
            
            ServerRPC_HandleGameplayEvent(inputTag);
        }
        else
        {
            NET_LOG(LogTemp, Warning, TEXT("후속 입력 대기 중이라 일반 입력 [%s] 무시됨"), *inputTag.ToString());
        }
        return true;
    }
    
    return false;
}

bool UAgentAbilitySystemComponent::IsValidFollowUpInput(const FGameplayTag& InputTag) const
{
    return CurrentFollowUpInputs.Contains(InputTag);
}

void UAgentAbilitySystemComponent::BroadcastStateChange(FGameplayTag NewState)
{
    OnAbilityStateChanged.Broadcast(NewState);
}

void UAgentAbilitySystemComponent::MulticastRPC_NotifyAbilityStateChanged_Implementation(FGameplayTag StateTag, bool bApply)
{
    // 클라이언트에서 상태 동기화 (서버에서는 이미 설정됨)
    if (GetOwnerRole() != ROLE_Authority)
    {
        if (bApply)
        {
            AddLooseGameplayTag(StateTag);
        }
        else
        {
            RemoveLooseGameplayTag(StateTag);
        }
        BroadcastStateChange(StateTag);
    }
}

void UAgentAbilitySystemComponent::MulticastRPC_NotifyAnimationCompleted_Implementation()
{
    // 모든 클라이언트에서 애니메이션 완료 처리
    FGameplayAbilitySpec* FoundSpec = nullptr;
    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (Spec.IsActive())
        {
            FoundSpec = &Spec;
            break;
        }
    }

    if (FoundSpec)
    {
        UBaseGameplayAbility* AbilityInstance = Cast<UBaseGameplayAbility>(FoundSpec->GetPrimaryInstance());
        if (AbilityInstance)
        {
            AbilityInstance->OnReadyAnimationCompleted();
        }
    }
}