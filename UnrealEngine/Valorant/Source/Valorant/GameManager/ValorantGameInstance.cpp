// Fill out your copyright notice in the Description page of Project Settings.


#include "ValorantGameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "ResourceManager/ValorantGameType.h"

void UValorantGameInstance::Init()
{
	Super::Init();
	
	FValorantGameplayTags::Get().InitializeNativeTags();

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

	// // Game Effect Data
	// UDataTable* GEffectData = LoadObject<UDataTable>(
	// 	nullptr, TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Weapon.dt_Weapon'"));
	// if (nullptr != WeaponData)
	// {
	// 	TArray<FName> RowNames = GEffectData->GetRowNames();
	//
	// 	for (const FName& RowName : RowNames)
	// 	{
	// 		FGameplayEffectData* Row = GEffectData->FindRow<FGameplayEffectData>(RowName, TEXT(""));
	// 		if (nullptr != Row)
	// 		{
	// 			dt_GEffect.Add(Row->EffectID, *Row);
	// 		}
	// 	}
	// }
	//
	// // Ability Data
	// UDataTable* AbilityData = LoadObject<UDataTable>(
	// 	nullptr, TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Weapon.dt_Weapon'"));
	// if (nullptr != WeaponData)
	// {
	// 	TArray<FName> RowNames = AbilityData->GetRowNames();
	//
	// 	for (const FName& RowName : RowNames)
	// 	{
	// 		FAbilityData* Row = AbilityData->FindRow<FAbilityData>(RowName, TEXT(""));
	// 		if (nullptr != Row)
	// 		{
	// 			dt_Ability.Add(Row->AbilityID, *Row);
	// 		}
	// 	}
	// }

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("Subsystem %s"), *OnlineSubsystem->GetSubsystemName().ToString())
			);
		}
	}
}

FAgentData* UValorantGameInstance::GetAgentData(int AgentID)
{
	if (FAgentData* data = dt_Agent.Find(AgentID))
	{
		return data;
	}
	UE_LOG(LogTemp, Error, TEXT("해당 ID의 데이터를 로드할 수 없습니다. AgentID:%d"),AgentID);
	return nullptr;
}

FWeaponData* UValorantGameInstance::GetWeaponData(int WeaponID)
{
	if (FWeaponData* data = dt_Weapon.Find(WeaponID))
	{
		return data;
	}
	UE_LOG(LogTemp, Error, TEXT("해당 ID의 데이터를 로드할 수 없습니다. WeaponID:%d"),WeaponID);
	return nullptr;
}

FGameplayEffectData* UValorantGameInstance::GetGEffectData(int GEffectID)
{
	if (FGameplayEffectData* data = dt_GEffect.Find(GEffectID))
	{
		return data;
	}
	UE_LOG(LogTemp, Error, TEXT("해당 ID의 데이터를 로드할 수 없습니다. GEffectID:%d"),GEffectID);
	return nullptr;
}

FAbilityData* UValorantGameInstance::GetAbilityData(int AbilityID)
{
	if (FAbilityData* data = dt_Ability.Find(AbilityID))
	{
		return data;
	}
	UE_LOG(LogTemp, Error, TEXT("해당 ID의 데이터를 로드할 수 없습니다. AbilityID:%d"),AbilityID);
	return nullptr;
}
