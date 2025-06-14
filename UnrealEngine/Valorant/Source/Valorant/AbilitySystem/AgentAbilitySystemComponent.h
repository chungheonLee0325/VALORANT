#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ValorantGameplayTags.h"
#include "Abilities/BaseGameplayAbility.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "AgentAbilitySystemComponent.generated.h"

class UValorantGameInstance;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStateChanged, FGameplayTag, StateTag);

UCLASS()
class VALORANT_API UAgentAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UAgentAbilitySystemComponent();
    
    // 초기화
    void InitializeByAgentData(int32 agentID);
    
    // 어빌리티 관리
    UFUNCTION(BlueprintCallable)
    void SetAgentAbility(int32 abilityID, int32 level);
    
    UFUNCTION(BlueprintCallable)
    void ResetAgentAbilities();
    
    // 어빌리티 정보 조회 - FGameplayAbilitySpec에서 직접 가져옴
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbilityDataBySlot(const FGameplayTag& SlotTag) const;
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_C() const;
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_E() const;
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_Q() const;
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_X() const;
    
    // 스킬 활성화
    UFUNCTION(BlueprintCallable)
    bool TryActivateAbilityByTag(const FGameplayTag& InputTag);
    
    // 상태 조회 헬퍼
    UFUNCTION(BlueprintPure, Category = "Ability|State")
    bool IsAbilityActive() const;
    
    // 이벤트
    UPROPERTY(BlueprintAssignable)
    FOnAbilityStateChanged OnAbilityStateChanged;
    
    // 어빌리티 강제 정리
    UFUNCTION(BlueprintCallable)
    void ForceCleanupAllAbilities();

    // 네트워크 동기화 - 시각적 피드백용
    UFUNCTION(NetMulticast, Reliable)
    void MulticastRPC_OnAbilityExecuted(FGameplayTag AbilityTag, bool bSuccess);

    UFUNCTION(Server, Reliable)
    void ServerRPC_SendGameplayEvent(FGameplayTag EventTag, const FGameplayEventData Payload);
    
protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
private:
    UPROPERTY()
    UValorantGameInstance* m_GameInstance = nullptr;
    
    // 스킬 태그 집합
    TSet<FGameplayTag> SkillTags = {
        FValorantGameplayTags::Get().InputTag_Ability_C,
        FValorantGameplayTags::Get().InputTag_Ability_E,
        FValorantGameplayTags::Get().InputTag_Ability_Q,
        FValorantGameplayTags::Get().InputTag_Ability_X
    };
    
    // 에이전트 ID만 저장
    UPROPERTY(Replicated)
    int32 m_AgentID;
    
    // 초기화 헬퍼
    void InitializeAttribute(const FAgentData* agentData);
    void RegisterAgentAbilities(const FAgentData* agentData);
    
    // 어빌리티 스펙 조회 헬퍼
    const FGameplayAbilitySpec* GetAbilitySpecByTag(const FGameplayTag& SlotTag) const;
};