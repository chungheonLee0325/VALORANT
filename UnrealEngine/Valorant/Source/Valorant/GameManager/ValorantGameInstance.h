// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// #define DEBUGTEST

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ResourceManager/ValorantGameType.h"
#include "Web/DatabaseManager.h"
#include "ValorantGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UValorantGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;

	/*
	 *	LoadingScreen
	 */
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> GameStartUpLoadingWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> LobbyToSelectLoadingWidgetClass;

protected:
	UFUNCTION()
	virtual void BeginLoadingScreen(const FString& MapName);
	UFUNCTION()
	virtual void EndLoadingScreen(UWorld* InLoadedWorld);
	
private:
	UPROPERTY()
	UUserWidget* CurrentLoadingWidget;
	
	/*
	 *	OnlineSubsystem
	 */
public:
	FTimerHandle CheckSessionHandle;
	bool bIsFindingMatch = false;
	bool bIsHostingMatch = false;
	int CurrentPlayerCount = 0;
	int MaxPlayerCount = 8;
	// TODO: 추후 삭제, 테스트를 위해 사용
	int ReqMatchAutoStartPlayerCount = 2;
	FOnGetPlayerCompleted OnGetPlayerCompletedDelegate;
	FString PlayerId = "";
	FString Platform = "";
	UFUNCTION()
	void OnGetPlayerCompleted(const bool bIsSuccess, const FPlayerDTO& PlayerDto);

	/*
	 *	DataTable
	 */
public:
	FAgentData* GetAgentData(int AgentID);
	FWeaponData* GetWeaponData(int WeaponID);
	void GetAllWeaponData(TArray<FWeaponData*>& WeaponList);
	FGameplayEffectData* GetGEffectData(int GEffectID);
	FAbilityData* GetAbilityData(int AbilityID);
	
	static UValorantGameInstance* Get(class UWorld* World);

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