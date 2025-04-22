// Fill out your copyright notice in the Description page of Project Settings.

#include "ValorantGameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Valorant.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Online/OnlineSessionNames.h"
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

void UValorantGameInstance::FindMatch()
{
	if (true == bIsFindingMatch)
	{
		return;
	}
	
	bIsFindingMatch = true;
	FindSessions();
}

void UValorantGameInstance::Init()
{
	Super::Init();
	
	FValorantGameplayTags::Get().InitializeNativeTags();

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

	if (false == SessionInterface.IsValid())
	{
		// OnlineSubsystem
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
}

void UValorantGameInstance::Shutdown()
{
	Super::Shutdown();
	
	DestroySession(NAME_GameSession);
}

void UValorantGameInstance::CreateSession()
{
	const FName SessionName = NAME_GameSession;
	if (false == SessionInterface.IsValid() && nullptr != SessionInterface->GetNamedSession(SessionName))
	{
		return;
	}

	FOnlineSessionSettings SessionSettings;
	// 기본적으로 LANMatch는 비활성화
	SessionSettings.bIsLANMatch = false;
	// 게임에 존재할 수 있는 최대 플레이어 수
	SessionSettings.NumPublicConnections = 8;
	// Steam 등의 온라인 서비스 사용 시 검색에 노출되도록 설정
	SessionSettings.bShouldAdvertise = true;
	// 같은 지역의 플레이어만 참가할 수 있도록 설정 (Steam 다운로드 지역 개인 설정 통일 필요할 수 있음)
	SessionSettings.bUsesPresence = true;
	// Lobby 방식 사용 설정
	SessionSettings.bUseLobbiesIfAvailable = true;
	// 난입은 불가 (세션이 Pending 상태{메치메이킹 중}일 때만 Join 가능)
	SessionSettings.bAllowJoinInProgress = false;
	// 세션에 Steam 친구 등을 초대할 수 있도록 설정
	SessionSettings.bAllowInvites = true;
	// 1로 설정하면 여러 유저가 각각 고유의 빌드 ID를 가지고 호스팅 및 게임 참가가 가능하다고 한다
	SessionSettings.BuildUniqueId = 1;

	// 만약, Subsystem이 NULL이라면,
	if (true == Online::GetSubsystem(GetWorld())->GetSubsystemName().IsEqual(TEXT("NULL")))
	{
		SessionSettings.bIsLANMatch = true;
		SessionSettings.bUseLobbiesIfAvailable = false;
	}
	
	// 특정 게임 모드로만 검색되고 접속할 수 있도록 Key-Value 방식의 태그를 걸 수도 있다.
	// 반드시 EOnlineDataAdvertisementType::ViaOnlineServiceAndPing로 설정해야 필터링이 된다.
	SessionSettings.Set(FName(TEXT("MATCH_TYPE")), FString(TEXT("SpikeRush")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// 상기 옵션으로 세션을 생성한다
	SessionInterface->CreateSession(0, SessionName, SessionSettings);
}

void UValorantGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete SessionName: %s, bWasSuccessful: %hs"), *SessionName.ToString(), bWasSuccessful?"True":"False");
}

void UValorantGameInstance::FindSessions()
{
	if (false == SessionInterface.IsValid())
	{
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	// 기본적으로 LANMatch는 비활성화
	SessionSearch->bIsLanQuery = false;
	// 세션 검색 결과는 최대 20개까지 설정
	SessionSearch->MaxSearchResults = 20;
	
	// 만약, Subsystem이 NULL이라면,
	if (true == Online::GetSubsystem(GetWorld())->GetSubsystemName().IsEqual(TEXT("NULL")))
	{
		SessionSearch->bIsLanQuery = true;
	}
	else
	{
		// bUseLobbiesIfAvailable 방식으로 세션을 생성하고 있으므로, 그것만 찾는다.
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}
	
	// Key-Value값(매치타입)이 일치하는 세션만 찾는다.
	SessionSearch->QuerySettings.Set(FName(TEXT("MATCH_TYPE")), FString(TEXT("SpikeRush")), EOnlineComparisonOp::Equals);
	// 상기 검색 조건에 맞는 세션만 찾는다.
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UValorantGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete bWasSuccessful: %hs"), bWasSuccessful?"True":"False");
	if (false == bWasSuccessful || false == SessionSearch.IsValid())
	{
		return;
	}

	const int FindSessionNum = SessionSearch->SearchResults.Num();
	NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete FindSessionNum: %d"), FindSessionNum);
	if (0 == FindSessionNum && true == bIsFindingMatch)
	{
		// 찾은 세션이 아무것도 없고 현재 매치를 찾는 중이라면 매치를 직접 만든다.
		CreateSession();
	}
	else
	{
		for (auto SearchResult : SessionSearch->SearchResults)
		{
			FString SessionID = SearchResult.GetSessionIdStr();
			FString HostName = SearchResult.Session.OwningUserName;
			const int32 RemSlotCount = SearchResult.Session.NumOpenPublicConnections;
			const int32 MaxSlotCount = SearchResult.Session.SessionSettings.NumPublicConnections;
			const int32 NumPlayers = MaxSlotCount - RemSlotCount;
			const bool bIsLanMatch = SearchResult.Session.SessionSettings.bIsLANMatch;
			NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete SessionID: %s, HostName: %s, (%d / %d), bIsLANMatch: %hs"), *SessionID, *HostName, NumPlayers, MaxSlotCount, bIsLanMatch?"True":"False");
			if (RemSlotCount > 0)
			{
				auto Session = SearchResult.Session;
				if (const bool bSuccess = SessionInterface->JoinSession(0, NAME_GameSession, SearchResult))
				{
					NET_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Try Join Session"));
					break;
				}
			}
		}
	}
}

void UValorantGameInstance::DestroySession(const FName SessionName)
{
	if (SessionInterface.IsValid())
	{
		if (SessionInterface->GetNamedSession(SessionName))
		{
			NET_LOG(LogTemp, Warning, TEXT("DestroySession SessionName: %s"), *SessionName.ToString());
			SessionInterface->DestroySession(SessionName);
		}
	}
}

void UValorantGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	NET_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete SessionName: %s, bWasSuccessful: %hs"), *SessionName.ToString(), bWasSuccessful?"True":"False");
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
