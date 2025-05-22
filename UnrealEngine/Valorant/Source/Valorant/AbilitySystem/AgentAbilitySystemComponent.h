// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "InputTriggers.h"
#include "ValorantGameplayTags.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "AgentAbilitySystemComponent.generated.h"

class UGameplayAbilityWithTag;
class UValorantGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityWaitingStateChanged, bool, bIsWaitingAbility);

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

    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_C() { return m_Ability_C; }
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_E() { return m_Ability_E; }
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_Q() { return m_Ability_Q; }
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_X() { return m_Ability_X; }

    UFUNCTION(BlueprintCallable)
    void ResisterFollowUpInput(const TSet<FGameplayTag>& tags);
    
    UFUNCTION(BlueprintCallable)
    void ResetFollowUpInput();

    UFUNCTION(BlueprintCallable)
    bool TrySkillInput(const FGameplayTag& inputTag);

    UFUNCTION(BlueprintCallable)
    void SetSkillClear(const bool isClear) { bIsSkillClear = isClear; }
    UFUNCTION(BlueprintCallable)
    void SetSkillReady(const bool isReady) { bIsSkillReady = isReady; }
    
    UPROPERTY(BlueprintAssignable)
    FOnAbilityWaitingStateChanged OnAbilityWaitingStateChanged;

private:
    UPROPERTY()
    UValorantGameInstance* m_GameInstance = nullptr;
    
    TSet<FGameplayTag> SkillTags = {
        FValorantGameplayTags::Get().InputTag_Ability_C,
        FValorantGameplayTags::Get().InputTag_Ability_E,
        FValorantGameplayTags::Get().InputTag_Ability_Q,
        FValorantGameplayTags::Get().InputTag_Ability_X
    };
    
    UPROPERTY(VisibleAnywhere)
    TSet<FGameplayTag> FollowUpInputBySkill;
    
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

    bool bIsSkillClear = true;
    UPROPERTY(Replicated)
    bool bIsSkillReady = false;
    
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;
    
    //AttributeSet
    void InitializeAttribute(const FAgentData* agentData);

    //Skill Input
    bool IsFollowUpInput(const FGameplayTag& inputTag);

    UFUNCTION(Server, Reliable)
    void ServerRPC_HandleGameplayEvent(const FGameplayTag& inputTag);
};