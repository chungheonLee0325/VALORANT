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
	void SetCurrentAbilityHandle(const FGameplayAbilitySpecHandle handle);
	
	UFUNCTION(BlueprintCallable)
	void ClearCurrentAbilityHandle(const FGameplayAbilitySpecHandle handle);

	UFUNCTION(BlueprintCallable)
	void ResisterFollowUpInput(const TSet<FGameplayTag>& tags);

	UFUNCTION(BlueprintCallable)
	bool TrySkillInput(const FGameplayTag& inputTag);

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
	
	// UPROPERTY(VisibleAnywhere)
	// TMap<FGameplayTag, FGameplayAbilitySpecHandle> ReservedSkillHandleMap;
	
	UPROPERTY(VisibleAnywhere)
	FGameplayAbilitySpecHandle CurrentAbilityHandle;
	
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

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//AttributeSet
	void InitializeAttribute(const FAgentData* agentData);

	//Skill
	UFUNCTION(NetMulticast, Reliable)
	void Net_ReserveSkill(const FGameplayTag& skillTag, const FGameplayAbilitySpecHandle& handle);
	UFUNCTION(NetMulticast, Reliable)
	void Net_ResetSkill(const TArray<FGameplayTag>& tagsToRemove);

	//Skill Input
	bool IsFollowUpInput(const FGameplayTag& inputTag);

	bool TrySkillFollowupInput(const FGameplayTag& inputTag);
};

