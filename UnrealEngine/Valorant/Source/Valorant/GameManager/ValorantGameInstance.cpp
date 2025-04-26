// Fill out your copyright notice in the Description page of Project Settings.

#include "ValorantGameInstance.h"

#include "MoviePlayer.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "SubsystemSteamManager.h"
#include "Valorant.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h" 
#include "ResourceManager/ValorantGameType.h"

template<typename RowStructType, typename IDType>
void LoadDataTableToMap(const FString& Path, TMap<IDType, RowStructType>& OutMap, IDType RowStructType::* IDMember)
{
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *Path);
	if (DataTable)
	{
		TArray<FName> RowNames = DataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			RowStructType* Row = DataTable->FindRow<RowStructType>(RowName, TEXT(""));
			if (Row != nullptr)
			{
				OutMap.Add(Row->*IDMember, *Row);
			}
		}
	}
}

void UValorantGameInstance::Init()
{
	Super::Init();
	FValorantGameplayTags::Get().InitializeNativeTags();
	
	/*
	 *	LoadingScreen
	 */
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UValorantGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UValorantGameInstance::EndLoadingScreen);

	/*
	 *	DataTable
	 */
	// Agent Data Load
	LoadDataTableToMap<FAgentData, int32>(
		TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Agent.dt_Agent'"),
		dt_Agent,
		&FAgentData::AgentID
	);

	// Weapon Data Load
	LoadDataTableToMap<FWeaponData, int32>(
		TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Weapon.dt_Weapon'"),
		dt_Weapon,
		&FWeaponData::WeaponID
	);

	// Agent Data Load
	LoadDataTableToMap<FAbilityData, int32>(
		TEXT("/Script/Engine.DataTable'/Game/BluePrint/DataTable/dt_Ability.dt_Ability'"),
		dt_Ability,
		&FAbilityData::AbilityID
	);

	/*
	 *	OnlineSubsystem
	 */
	const IOnlineSessionPtr SessionInterface = USubsystemSteamManager::GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		const FName SubsystemName = Online::GetSubsystem(GetWorld())->GetSubsystemName();
		UE_LOG(LogTemp, Warning, TEXT("SubsystemName : %s"), *SubsystemName.ToString());
	}
}

void UValorantGameInstance::BeginLoadingScreen(const FString& MapName)
{
	UE_LOG(LogTemp, Warning, TEXT("BeginLoadingScreen MapName: %s"), *MapName);
	if (!IsRunningDedicatedServer())
	{
		// 로딩하는 맵 종류에 따라 다른 UI를 표시한다.
		TSubclassOf<UUserWidget> WidgetClass;
		if (MapName.Contains(TEXT("MatchMap")))
		{
			WidgetClass = LobbyToSelectLoadingWidgetClass;
		}
		else
		{
			WidgetClass = GameStartUpLoadingWidgetClass;
		}
		
		if (CurrentLoadingWidget)
		{
			CurrentLoadingWidget->RemoveFromParent();
			CurrentLoadingWidget = nullptr;
		}
		
		CurrentLoadingWidget = CreateWidget<UUserWidget>(this, WidgetClass);
		if (CurrentLoadingWidget)
		{
			FLoadingScreenAttributes LoadingScreen;
			LoadingScreen.WidgetLoadingScreen = CurrentLoadingWidget->TakeWidget();
			LoadingScreen.bAllowInEarlyStartup = false;
			LoadingScreen.PlaybackType = MT_Normal;
			LoadingScreen.bAllowEngineTick = false;
			LoadingScreen.bWaitForManualStop = false;
			// 맵 로딩이 완료되면 자동으로 파괴한다. (그럼에도 RemoveFromParent를 해야 하는지는 검증 필요)
			LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
			// 최소한 3초는 표시한다.
			LoadingScreen.MinimumLoadingScreenDisplayTime = 3.f;
			
			GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
		}
	}
}

void UValorantGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	UE_LOG(LogTemp, Warning, TEXT("EndLoadingScreen"));
}

void UValorantGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	// NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete bWasSuccessful: %hs"), bWasSuccessful?"True":"False");
	// if (false == bWasSuccessful || false == SessionSearch.IsValid())
	// {
	// 	return;
	// }
	//
	// const int FindSessionNum = SessionSearch->SearchResults.Num();
	// NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete FindSessionNum: %d"), FindSessionNum);
	// if (0 == FindSessionNum && true == bIsFindingMatch)
	// {
	// 	// 찾은 세션이 아무것도 없고 현재 매치를 찾는 중이라면 매치를 직접 만든다.
	// 	bIsHostingMatch = true;
	// 	//GetWorld()->GetTimerManager().SetTimer(CheckSessionHandle, this, &UValorantGameInstance::CheckHostingSession, 1.0f, true);
	// 	//CreateSession();
	// }
	// else
	// {
	// 	for (const auto& SearchResult : SessionSearch->SearchResults)
	// 	{
	// 		FString SessionID = SearchResult.GetSessionIdStr();
	// 		FString HostName = SearchResult.Session.OwningUserName;
	// 		const int32 RemSlotCount = SearchResult.Session.NumOpenPublicConnections;
	// 		const int32 MaxSlotCount = SearchResult.Session.SessionSettings.NumPublicConnections;
	// 		const int32 NumPlayers = MaxSlotCount - RemSlotCount;
	// 		const bool bIsLanMatch = SearchResult.Session.SessionSettings.bIsLANMatch;
	// 		NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete SessionID: %s, HostName: %s, (%d / %d), bIsLANMatch: %hs"), *SessionID, *HostName, NumPlayers, MaxSlotCount, bIsLanMatch?"True":"False");
	// 		if (RemSlotCount > 0)
	// 		{
	// 			bool bUsesPresence = SearchResult.Session.SessionSettings.bUsesPresence;
	// 			bool bUseLobbiesIfAvailable = SearchResult.Session.SessionSettings.bUseLobbiesIfAvailable;
	// 			NET_LOG(LogTemp, Warning, TEXT("Before bUsesPresence: %hs, bUseLobbiesIfAvailable: %hs"), bUsesPresence?"True":"False", bUseLobbiesIfAvailable?"True":"False");
	// 			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// 			auto& SearchResultRef = const_cast<FOnlineSessionSearchResult&>(SearchResult);
	// 			auto& SessionSettings = SearchResultRef.Session.SessionSettings;
	// 			SessionSettings.bUsesPresence = true;
	// 			SessionSettings.bUseLobbiesIfAvailable = true;
	// 			bUsesPresence = SessionSettings.bUsesPresence;
	// 			bUseLobbiesIfAvailable = SessionSettings.bUseLobbiesIfAvailable;
	// 			NET_LOG(LogTemp, Warning, TEXT("After bUsesPresence: %hs, bUseLobbiesIfAvailable: %hs"), bUsesPresence?"True":"False", bUseLobbiesIfAvailable?"True":"False");
	// 			const bool bSuccess = SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult);
	// 			if (bSuccess) 
	// 			{
	// 				NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Try Join Session is Success"));
	// 				break;
	// 			}
	// 			else
	// 			{
	// 				NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Try Join Session is Failed"));
	// 			}
	// 		}
	// 	}
	// }
}

void UValorantGameInstance::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnUpdateSessionComplete SessionName: %s, bWasSuccessful: %hs"), *SessionName.ToString(), bWasSuccessful?"True":"False");
}

void UValorantGameInstance::BroadcastTravel()
{
	// if (false == SessionInterface.IsValid())
	// {
	// 	NET_LOG(LogTemp, Warning, TEXT("BroadcastTravel: SessionInterface is Invalid"));
	// 	return;
	// }
	//
	// auto* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	// if (nullptr == Session)
	// {
	// 	NET_LOG(LogTemp, Warning, TEXT("BroadcastTravel: Session is nullptr"));
	// 	return;
	// }
	//
	// NET_LOG(LogTemp, Warning, TEXT("BroadcastTravel: Try Update Session"));
	// Session->SessionSettings.Set(FName("bReadyToTravel"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// SessionInterface->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
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