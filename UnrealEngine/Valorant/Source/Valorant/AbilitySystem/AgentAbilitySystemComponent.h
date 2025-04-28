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
	void SetAgentAbility(TSubclassOf<UGameplayAbility> abilityClass, int32 level);

	UFUNCTION(BlueprintCallable)
	void ResetAgentAbilities();
	
	//Skill
	UFUNCTION(BlueprintCallable)
	void SetCurrentAbilityHandle(const FGameplayAbilitySpecHandle handle);
	
	UFUNCTION(BlueprintCallable)
	void ClearCurrentAbilityHandle(const FGameplayAbilitySpecHandle handle);

	UFUNCTION(BlueprintCallable)
	void ResisterFollowUpInput(const TSet<FGameplayTag>& tags);

	UFUNCTION(BlueprintCallable)
	bool TrySkillInput(const FGameplayTag& inputTag);

	//TODO: 스킬 Data 가지고 있기
	FString GetAgentName() const { return AgentName; }
	FString GetSkillQName() const { return SkillQName; }
	FString GetSkillEName() const { return SkillEName; }
	FString GetSkillCName() const { return SkillCName; }
	FString GetSkillXName() const { return SkillXName; }

	UPROPERTY(BlueprintAssignable)
	FOnAbilityWaitingStateChanged OnAbilityWaitingStateChanged;

	
private:
	TSet<FGameplayTag> SkillTags = {
		FValorantGameplayTags::Get().InputTag_Ability_Q,
		FValorantGameplayTags::Get().InputTag_Ability_E,
		FValorantGameplayTags::Get().InputTag_Ability_C,
		FValorantGameplayTags::Get().InputTag_Ability_X
	};
	
	UPROPERTY(VisibleAnywhere)
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ReservedSkillHandleMap;
	
	UPROPERTY(VisibleAnywhere)
	FGameplayAbilitySpecHandle CurrentAbilityHandle;
	
	UPROPERTY(VisibleAnywhere)
	TSet<FGameplayTag> FollowUpInputBySkill;
	
	UPROPERTY(Replicated)
	int32 m_AgentID;

	//TODO: 스킬 데이터 따로 담기
	UPROPERTY(Replicated)
	FString AgentName = "";
	UPROPERTY(Replicated)
	FString SkillQName = "";
	UPROPERTY(Replicated)
	FString SkillEName = "";
	UPROPERTY(Replicated)
	FString SkillCName = "";
	UPROPERTY(Replicated)
	FString SkillXName = "";

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//AttributeSet
	void InitializeAttribute(const FAgentData* agentData);

	//Skill
	UFUNCTION(Client, Reliable)
	void Client_ReserveSkill(const FGameplayTag& skillTag, const FGameplayAbilitySpecHandle& handle);
	UFUNCTION(Client, Reliable)
	void Client_ResetSkill(const TArray<FGameplayTag>& tagsToRemove);

	//Skill Input
	bool IsFollowUpInput(const FGameplayTag& inputTag);

	bool TrySkillFollowupInput(const FGameplayTag& inputTag);
	
	//TODO: 스킬 충전하는 함수
	//TODO: 스킬 정보 넘기는 함수
};

