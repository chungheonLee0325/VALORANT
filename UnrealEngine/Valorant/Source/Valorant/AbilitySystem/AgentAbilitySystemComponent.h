#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "InputTriggers.h"
#include "ValorantGameplayTags.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "AgentAbilitySystemComponent.generated.h"

class UGameplayAbilityWithTag;
class UValorantGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStateChanged, FGameplayTag, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityPhaseChanged, FGameplayTag, AbilityTag, FGameplayTag, NewPhase);

UCLASS()
class VALORANT_API UAgentAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UAgentAbilitySystemComponent();
    
    /**서버에서만 호출됩니다.*/
    void InitializeByAgentData(int32 agentID);

    //Ability 등록 및 해제
    void RegisterAgentAbilities(const FAgentData* agentData);
    
    UFUNCTION(BlueprintCallable)
    void SetAgentAbility(int32 abilityID, int32 level);

    UFUNCTION(BlueprintCallable)
    void ResetAgentAbilities();

    // 기존 어빌리티 정보 접근자들 (하위 호환성)
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_C() { return m_Ability_C; }
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_E() { return m_Ability_E; }
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_Q() { return m_Ability_Q; }
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_X() { return m_Ability_X; }

    // === 개선된 태그 기반 상태 관리 ===
    
    // 어빌리티 상태 확인 (기존 bool 함수들을 대체)
    UFUNCTION(BlueprintCallable, Category = "Ability|State")
    bool IsAbilityExecuting() const;
    
    UFUNCTION(BlueprintCallable, Category = "Ability|State")
    bool IsAbilityReady() const;
    
    UFUNCTION(BlueprintCallable, Category = "Ability|State")
    bool IsWaitingForFollowUp() const;
    
    UFUNCTION(BlueprintCallable, Category = "Ability|State")
    bool CanActivateAbilities() const;
    
    // 어빌리티 상태 설정 (기존 SetSkillClear/SetSkillReady 대체)
    UFUNCTION(BlueprintCallable, Category = "Ability|State")
    void SetAbilityState(FGameplayTag StateTag, bool bApply = true);
    
    // 후속 입력 관리 (개선된 버전)
    UFUNCTION(BlueprintCallable, Category = "Ability|Input")
    void RegisterFollowUpInputs(const TSet<FGameplayTag>& InputTags, FGameplayTag AbilityTag);
    
    UFUNCTION(BlueprintCallable, Category = "Ability|Input")
    void ClearFollowUpInputs();
    
    UFUNCTION(BlueprintCallable, Category = "Ability|Input")
    bool TrySkillInput(const FGameplayTag& InputTag);
    
    // 기존 함수들 (하위 호환성을 위해 유지하되, 내부적으로 태그 사용)
    UFUNCTION(BlueprintCallable)
    void SetSkillClear(const bool isClear);
    
    UFUNCTION(BlueprintCallable)
    void SetSkillReady(const bool isReady);
    
    UFUNCTION(BlueprintCallable)
    void ResisterFollowUpInput(const TSet<FGameplayTag>& tags);
    
    UFUNCTION(BlueprintCallable)
    void ResetFollowUpInput();
    
    // 이벤트 델리게이트들
    UPROPERTY(BlueprintAssignable)
    FOnAbilityStateChanged OnAbilityStateChanged;
    
    UPROPERTY(BlueprintAssignable)
    FOnAbilityPhaseChanged OnAbilityPhaseChanged;
    
    // 기존 델리게이트 (하위 호환성)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityWaitingStateChanged, bool, bIsWaitingAbility);
    UPROPERTY(BlueprintAssignable)
    FOnAbilityWaitingStateChanged OnAbilityWaitingStateChanged;

private:
    UPROPERTY()
    UValorantGameInstance* m_GameInstance = nullptr;
    
    // 어빌리티 태그 매핑
    UPROPERTY()
    TMap<FGameplayTag, FAbilityData> AbilityDataMap;
    
    // 현재 후속 입력 대기 정보
    UPROPERTY()
    TSet<FGameplayTag> CurrentFollowUpInputs;
    
    UPROPERTY()
    FGameplayTag CurrentExecutingAbility;
    
    // 기존 SkillTags (하위 호환성)
    TSet<FGameplayTag> SkillTags = {
        FValorantGameplayTags::Get().InputTag_Ability_C,
        FValorantGameplayTags::Get().InputTag_Ability_E,
        FValorantGameplayTags::Get().InputTag_Ability_Q,
        FValorantGameplayTags::Get().InputTag_Ability_X
    };
    
    // 기존 변수들 (하위 호환성을 위해 유지하되, 태그로 대체)
    UPROPERTY(VisibleAnywhere)
    TSet<FGameplayTag> FollowUpInputBySkill;
    
    // 기존 replicated 변수들 (하위 호환성을 위해 유지)
    UPROPERTY(Replicated)
    int32 m_AgentID;
    
    UPROPERTY(Replicated)
    FAbilityData m_Ability_C;
    UPROPERTY(Replicated)
    FAbilityData m_Ability_E;
    UPROPERTY(Replicated)
    FAbilityData m_Ability_Q;
    UPROPERTY(Replicated)
    FAbilityData m_Ability_X;

    // bool 변수들은 더 이상 사용하지 않지만 하위 호환성을 위해 유지
    bool bIsSkillClear = true;
    UPROPERTY(Replicated)
    bool bIsSkillReady = false;
    
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;
    
    // AttributeSet 초기화
    void InitializeAttribute(const FAgentData* agentData);

    // 내부 헬퍼 함수들
    bool IsValidFollowUpInput(const FGameplayTag& InputTag) const;
    void BroadcastStateChange(FGameplayTag NewState);
    void CleanupAbilityState();
    
    // 기존 함수 (하위 호환성)
    bool IsFollowUpInput(const FGameplayTag& inputTag);

    UFUNCTION(Server, Reliable)
    void ServerRPC_HandleGameplayEvent(const FGameplayTag& inputTag);
    
    UFUNCTION(Server, Reliable)
    void ServerRPC_SetAbilityState(FGameplayTag StateTag, bool bApply);
};