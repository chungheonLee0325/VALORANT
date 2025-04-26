// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ResourceManager/ValorantGameType.h"
#include "ValorantGameInstance.generated.h"

class ULoadingTestUI;
/**
 * 
 */
UCLASS()
class VALORANT_API UValorantGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> SessionInterface;
	// TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FTimerHandle CheckSessionHandle;
	bool bIsFindingMatch = false;
	bool bIsHostingMatch = false;
	int CurrentPlayerCount = 0;
	int MaxPlayerCount = 8;
	// TODO: 추후 삭제, 테스트를 위해 사용
	int ReqMatchAutoStartPlayerCount = 2;
	void BroadcastTravel();
	
protected:
	virtual void Init() override;
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnUpdateSessionComplete(FName Name, bool bArg);

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

	/*
	 *	LoadingScreen
	 */
public:
	UFUNCTION()
	virtual void BeginLoadingScreen(const FString& MapName);
	UFUNCTION()
	virtual void EndLoadingScreen(UWorld* InLoadedWorld);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> GameStartUpLoadingWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> LobbyToSelectLoadingWidgetClass;

private:
	UPROPERTY()
	UUserWidget* CurrentLoadingWidget;
};
