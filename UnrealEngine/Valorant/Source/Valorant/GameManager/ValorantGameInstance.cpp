// Fill out your copyright notice in the Description page of Project Settings.


#include "ValorantGameInstance.h"

#include "Valorant/ResourceManager/ValorantGameType.h"

void UValorantGameInstance::Init()
{
	Super::Init();

	// Agent Data
	UDataTable* AgentData = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Agent.dt_Agent'"));
	if (nullptr != AgentData)
	{
		TArray<FName> RowNames = AgentData->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FAgentData* Row = AgentData->FindRow<FAgentData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Agent.Add(Row->AgentID, *Row);
			}
		}
	}

	// Weapon Data
	UDataTable* WeaponData = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Weapon.dt_Weapon'"));
	if (nullptr != WeaponData)
	{
		TArray<FName> RowNames = WeaponData->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FWeaponData* Row = WeaponData->FindRow<FWeaponData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Weapon.Add(Row->WeaponID, *Row);
			}
		}
	}
}

FAgentData* UValorantGameInstance::GetAgentData(int AgentID)
{
	if (FAgentData* data = dt_Agent.Find(AgentID))
	{
		return data;
	}
	return nullptr;
}

FWeaponData* UValorantGameInstance::GetWeaponData(int WeaponID)
{
	if (FWeaponData* data = dt_Weapon.Find(WeaponID))
	{
		return data;
	}
	return nullptr;
}
