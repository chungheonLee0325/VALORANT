// Fill out your copyright notice in the Description page of Project Settings.


#include "ValorantGameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Valorant.h"
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
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		NET_LOG(LogTemp, Warning, TEXT("SubsystemName : %s"), *OnlineSubsystem->GetSubsystemName().ToString());
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UValorantGameInstance::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UValorantGameInstance::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UValorantGameInstance::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UValorantGameInstance::OnJoinSessionComplete);
		}
	}
}

void UValorantGameInstance::Shutdown()
{
	Super::Shutdown();
	
	DestroySession(NAME_GameSession);
}

void UValorantGameInstance::CreateSession()
{
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings;
		SessionSettings.NumPublicConnections = 2;
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;
		SessionSettings.bUseLobbiesIfAvailable = true;
		SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
	}
}

void UValorantGameInstance::FindSessions()
{
	if (false == SessionInterface.IsValid())
	{
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 5;
	SessionSearch->QuerySettings.Set(TEXT("presence"), true, EOnlineComparisonOp::Equals);
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UValorantGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete SessionName: %s, bWasSuccessful: %hs"), *SessionName.ToString(), bWasSuccessful?"True":"False");
}

void UValorantGameInstance::DestroySession(const FName SessionName)
{
	if (SessionInterface.IsValid() && nullptr != SessionInterface->GetNamedSession(SessionName))
	{
		SessionInterface->DestroySession(SessionName);
	}
}

void UValorantGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete SessionName: %s, bWasSuccessful: %hs"), *SessionName.ToString(), bWasSuccessful?"True":"False");
}

void UValorantGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete bWasSuccessful: %hs, Num: "), bWasSuccessful?"True":"False");
	if (false == bWasSuccessful || false == SessionSearch.IsValid())
	{
		return;
	}

	NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete FindSessionNum: %d"), SessionSearch->SearchResults.Num());
	for (auto SearchResult : SessionSearch->SearchResults)
	{
		FString SessionID = SearchResult.GetSessionIdStr();
		FString HostName = SearchResult.Session.OwningUserName;
		int32 NumPlayers = SearchResult.Session.SessionSettings.NumPublicConnections - SearchResult.Session.NumOpenPublicConnections;
		int32 MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
		bool bIsLANMatch = SearchResult.Session.SessionSettings.bIsLANMatch;
		UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete SessionID: %s, HostName: %s, NumPlayers: %d, MaxPlayers: %d, bIsLANMatch: %hs"), *SessionID, *HostName, NumPlayers, MaxPlayers, bIsLANMatch?"True":"False");
			
	}
}

void UValorantGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	NET_LOG(LogTemp, Warning, TEXT("OnJoinSessionComplete SessionName: %s, Result: %d"), *SessionName.ToString(), Result);
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
