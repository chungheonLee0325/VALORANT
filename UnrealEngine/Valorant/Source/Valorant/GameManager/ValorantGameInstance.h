// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ResourceManager/ValorantGameType.h"
#include "ValorantGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UValorantGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

protected:
	virtual void Init() override;
	virtual void Shutdown() override;
	UFUNCTION(BlueprintCallable, meta=(AllowPrivateAccess = "true"))
	void CreateSession();
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	UFUNCTION(BlueprintCallable, meta=(AllowPrivateAccess = "true"))
	void FindSessions();
	void OnFindSessionsComplete(bool bWasSuccessful);
	void DestroySession(FName SessionName);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName Name, EOnJoinSessionCompleteResult::Type Arg);

public:
	FAgentData* GetAgentData(int AgentID);
	FWeaponData* GetWeaponData(int WeaponID);
	FGameplayEffectData* GetGEffectData(int GEffectID);
	FAbilityData* GetAbilityData(int AbilityID);
	
private:
	UPROPERTY()
	TMap<int32, FAgentData> dt_Agent;
	UPROPERTY()
	TMap<int32, FWeaponData> dt_Weapon;
	UPROPERTY()
	TMap<int32, FGameplayEffectData> dt_GEffect;
	UPROPERTY()
	TMap<int32, FAbilityData> dt_Ability;
};
