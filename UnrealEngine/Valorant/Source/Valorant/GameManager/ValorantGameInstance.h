// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "ValorantGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UValorantGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	FAgentData* GetAgentData(int AgentID);
	FWeaponData* GetWeaponData(int WeaponID);

protected:
	virtual void Init() override;

private:
	UPROPERTY()
	TMap<int32, FAgentData> dt_Agent;
	UPROPERTY()
	TMap<int32, FWeaponData> dt_Weapon;
};
