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



UCLASS()
class VALORANT_API UAgentAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UAgentAbilitySystemComponent();

	/**서버에서만 호출됩니다.*/
	void InitializeByAgentData(int32 agentID);
	
	UFUNCTION(BlueprintCallable)
	void SkillCallByTag(const FGameplayTag& inputTag);

	UFUNCTION(BlueprintCallable)
	void ResisterFollowUpInput(const TArray<FGameplayTag>& tags);

	UFUNCTION(BlueprintCallable)
	void ClearCurrnetAbility(const FGameplayAbilitySpecHandle& handle);
	
	FString GetAgentName() const { return AgentName; }
	FString GetSkillQName() const { return SkillQName; }
	FString GetSkillEName() const { return SkillEName; }
	FString GetSkillCName() const { return SkillCName; }
	FString GetSkillXName() const { return SkillXName; }
	
private:
	TSet<FGameplayTag> SkillTags = {
		FValorantGameplayTags::Get().InputTag_Ability_Q,
		FValorantGameplayTags::Get().InputTag_Ability_E,
		FValorantGameplayTags::Get().InputTag_Ability_C,
		FValorantGameplayTags::Get().InputTag_Ability_X
	};

	// 좌클릭 우클릭은 무조건 reserved map 먼저 확인후 nullptr이면 skillHandleMap 검색
	
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ReservedSkillHandleMap;
	
	// ToDo : ReservedSkillHandleMap - setter / reset method 필요

	// Set과 병행
	UPROPERTY()
	FGameplayAbilitySpecHandle CurrentAbilityHandle;

	// TSet으로 관리
	// 스킬 시전마다 Set 리셋하고 시작
	
	UPROPERTY()
	TArray<FGameplayTag> FollowUpInputBySkill;
	
	UPROPERTY(Replicated)
	int32 m_AgentID;

	//TODO: 데이터를 이렇게 담는 게 맞나?
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

	//Ability
	void RegisterAgentAbilities(const FAgentData* agentData);
	void GiveAgentAbility(TSubclassOf<UGameplayAbility> abilityClass, int32 level);
	void ClearAgentAbilities();
	void ClearAgentAbility(const FGameplayTagContainer& tags);
	void ClearAgentAbility(const FGameplayAbilitySpec* spec);

	//Skill Input
	void ClearFollowUpInput();
	
	bool IsFollowUpInput(const FGameplayTag& inputTag);

	bool TrySkillFollowupInput(const FGameplayTag& inputTag);

	UFUNCTION(BlueprintCallable)
	bool TrySkillInput(const FGameplayTag& inputTag);

	
	//TODO: 스킬 충전하는 함수
	//TODO: 스킬 정보 넘기는 함수
};
