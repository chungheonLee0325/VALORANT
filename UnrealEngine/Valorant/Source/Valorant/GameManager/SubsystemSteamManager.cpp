// Fill out your copyright notice in the Description page of Project Settings.


#include "SubsystemSteamManager.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

// Custom Log
DEFINE_LOG_CATEGORY(LogSubsystemSteam);

USubsystemSteamManager::USubsystemSteamManager() :
	OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &USubsystemSteamManager::OnCreateSessionComplete_Internal)),
	OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &USubsystemSteamManager::OnFindSessionComplete_Internal)),
	OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &USubsystemSteamManager::OnJoinSessionComplete_Internal)),
	OnDestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &USubsystemSteamManager::OnDestroySessionComplete_Internal))
{
}

IOnlineSessionPtr USubsystemSteamManager::GetSessionInterface()
{
	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && OnlineSubsystem->GetSessionInterface())
	{
		return OnlineSubsystem->GetSessionInterface();
	}
	UE_LOG(LogSubsystemSteam, Error, TEXT("Invalid Session Interface"));
	return nullptr;
}

void USubsystemSteamManager::CreateSession()
{
	LOG_DARK_YELLOW(TEXT("%hs Called"), __FUNCTION__);
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnCreateSteamSessionComplete.Broadcast(false);
		return;
	}

	// 이미 세션이 존재하면 세션 제거. 제거가 완료되면 다시 CreateSession() 호출됨
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		UE_LOG(LogSubsystemSteam, Warning, TEXT("%hs Called, Session already exists"), __FUNCTION__);
		bCreateSessionOnDestroy = true;
		DestroySession();
		return;
	}

	// Store the delegate in a FDelegateHandle so we can later remove it from the delegate list
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
	
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	// OnlineSubsystem을 사용하지 않으면 LANMatch
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == TEXT("NULL");

	// 게임에 존재할 수 있는 최대 플레이어의 수
	LastSessionSettings->NumPublicConnections = 2;	// TODO : Parameterize

	// 세션이 작동중일 때 다른 플레이어가 참가할 수 있는지 여부
	LastSessionSettings->bAllowJoinInProgress = false;
	
	// 스팀이 세션을 광고하여 다른 플레이어가 세션을 찾아서 참가할 수 있는지 여부
	LastSessionSettings->bShouldAdvertise = true;

	// 로비 방식 사용
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	
	// 1로 설정하면 여러 유저가 각각 고유의 빌드와 호스팅을 할 수 있다. 이후 유효한 게임 세션을 검색할 때 각각의 여러 세션들을 검색하고 참가할 수 있다. 만약 1이 아니면 다른 유저들의 세션들을 볼 수 없고 첫번째로 열리는 게임 세션에 참가하려고 할 것이다.
	LastSessionSettings->BuildUniqueId = 1;

	// Dev App Id로 480을 사용할 때 식별을 위한 데이터 추가 // TODO : Parameterize
	LastSessionSettings->Set(NAME_Identifier, SessionIdentifier, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	if (const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController())
	{
		if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
		{
			UE_LOG(LogSubsystemSteam, Error, TEXT("%hs Called, CreateSession failed"), __FUNCTION__);
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			OnCreateSteamSessionComplete.Broadcast(false);
		}	
	}
}
void USubsystemSteamManager::OnCreateSessionComplete_Internal(FName SessionName, bool bWasSuccessful)
{
	LOG_DARK_YELLOW(TEXT("%hs Called, SessionName %s"), __FUNCTION__, *SessionName.ToString());

	GetWorld()->GetTimerManager().SetTimer(CheckSessionHandle, this, &USubsystemSteamManager::CheckHostingSession, 1.0f, true);
	OnCreateSteamSessionComplete.Broadcast(bWasSuccessful);
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}
}

void USubsystemSteamManager::FindSessions()
{
	LOG_DARK_YELLOW(TEXT("%hs Called"), __FUNCTION__);
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnFindSteamSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = 10000;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == TEXT("NULL");
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	// Will DEPRECATE After UE5.5
	// SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	LastSessionSearch->QuerySettings.Set(NAME_Identifier, SessionIdentifier, EOnlineComparisonOp::Equals);
	
	if (const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController())
	{
		if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
		{
			UE_LOG(LogSubsystemSteam, Error, TEXT("%hs Called, FindSession Failed"), __FUNCTION__);
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
			OnFindSteamSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
			OnFindFirstSteamSessionComplete.Broadcast(FOnlineSessionSearchResult(), false);
		}	
	}
}
void USubsystemSteamManager::OnFindSessionComplete_Internal(bool bWasSuccessful)
{
	LOG_DARK_YELLOW(TEXT("%hs Called, SerachResults Num : %d"), __FUNCTION__, LastSessionSearch->SearchResults.Num());
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	}

	if (!bWasSuccessful)
	{
		OnFindFirstSteamSessionComplete.Broadcast(FOnlineSessionSearchResult(), false);
		OnFindSteamSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// 식별된 모든 결과 저장
	TArray<FOnlineSessionSearchResult> IdentifiedResults;
	
	// 첫번째 세션을 반환했는지 여부
	bool bBroadcastFirstSession = false;
	
	for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
	{
		FString ResultValue;
		Result.Session.SessionSettings.Get(NAME_Identifier, ResultValue);
		
		// Session 찾음
		if (ResultValue == SessionIdentifier)
		{
			auto& SearchResultRef = const_cast<FOnlineSessionSearchResult&>(Result);
			auto& SessionSettings = SearchResultRef.Session.SessionSettings;
			SessionSettings.bUsesPresence = true;
			SessionSettings.bUseLobbiesIfAvailable = true;
			
			if (!bBroadcastFirstSession)
			{
				// 첫번째 세션만 반환
				bBroadcastFirstSession = true;
				OnFindFirstSteamSessionComplete.Broadcast(Result, true);
			}

			// 식별된 모든 결과 저장
			IdentifiedResults.Add(Result);
		}
	}

	// 모든 결과 반환
	OnFindSteamSessionsComplete.Broadcast(IdentifiedResults, true);
}

void USubsystemSteamManager::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	LOG_DARK_YELLOW(TEXT("%hs Called"), __FUNCTION__);
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnJoinSteamSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
	
	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		UE_LOG(LogSubsystemSteam, Error, TEXT("%hs Called, JoinSession failed"), __FUNCTION__);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		OnJoinSteamSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}
void USubsystemSteamManager::OnJoinSessionComplete_Internal(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	LOG_DARK_YELLOW(TEXT("%hs Called, SessionName %s, Result %s"), __FUNCTION__, *SessionName.ToString(), LexToString(Result));
	
	OnJoinSteamSessionComplete.Broadcast(Result);

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		GetWorld()->GetTimerManager().SetTimer(CheckSessionHandle, this, &USubsystemSteamManager::CheckJoinSession, 1.0f, true);
	}
}

void USubsystemSteamManager::DestroySession()
{
	LOG_DARK_YELLOW(TEXT("%hs Called"), __FUNCTION__);
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnDestroySteamSessionComplete.Broadcast(false);
		return;
	}

	OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

	if (!GetNamedOnlineSession())
	{
		// 제거할 세션이 없으면 true 반환하고 끝냄
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		OnDestroySteamSessionComplete.Broadcast(true);
	}
	else if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		UE_LOG(LogSubsystemSteam, Error, TEXT("%hs Called, DestroySession failed"), __FUNCTION__);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		OnDestroySteamSessionComplete.Broadcast(false);
	}
}
void USubsystemSteamManager::OnDestroySessionComplete_Internal(FName SessionName, bool bWasSuccessful)
{
	LOG_DARK_YELLOW(TEXT("%hs Called, SessionName %s"), __FUNCTION__, *SessionName.ToString());
	
	OnDestroySteamSessionComplete.Broadcast(bWasSuccessful);
	
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	}

	// 세션을 성공적으로 삭제했고, 세션 생성 중에 지운 경우라면 다시 세션 생성
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession();
	}
}

void USubsystemSteamManager::CheckHostingSession()
{
	const auto* Session = GetNamedOnlineSession(NAME_GameSession);
	if (nullptr == Session)
	{
		UE_LOG(LogSubsystemSteam, Warning, TEXT("%hs Called, Session is nullptr"), __FUNCTION__);
		return;
	}
	
	const int32 RemSlotCount = Session->NumOpenPublicConnections;
	const int MaxPlayerCount = Session->SessionSettings.NumPublicConnections;
	const int CurrentPlayerCount = MaxPlayerCount - RemSlotCount;
	LOG_DARK_YELLOW(TEXT("%hs Called, Session Slot State : (%d / %d)"), __FUNCTION__, CurrentPlayerCount, MaxPlayerCount);

	if (ReqMatchAutoStartPlayerCount <= CurrentPlayerCount)
	{
		UE_LOG(LogSubsystemSteam, Warning, TEXT("매치 자동 시작을 위해 필요한 인원 수가 충족됨"));
		GetWorld()->GetTimerManager().ClearTimer(CheckSessionHandle);
		StartMatch();
	}
}
void USubsystemSteamManager::CheckJoinSession()
{
	const auto* Session = GetNamedOnlineSession(NAME_GameSession);
	if (nullptr == Session)
	{
		UE_LOG(LogSubsystemSteam, Warning, TEXT("%hs Called, Session is nullptr"), __FUNCTION__);
		return;
	}
	
	bool bReady = false;
	Session->SessionSettings.Get(FName(TEXT("bReadyToTravel")), bReady);
	if (bReady)
	{
		UE_LOG(LogSubsystemSteam, Warning, TEXT("%hs Called, Ready to ClientTravel"), __FUNCTION__);
		GetWorld()->GetTimerManager().ClearTimer(CheckSessionHandle);
		FString ConnectString;
		bool bSuccess = GetSessionInterface()->GetResolvedConnectString(NAME_GameSession, ConnectString);
		if (bSuccess)
		{
			UE_LOG(LogSubsystemSteam, Warning, TEXT("ClientTravel to %s"), *ConnectString);
			GetGameInstance()->GetFirstLocalPlayerController()->ClientTravel(ConnectString, TRAVEL_Absolute, false);
		}	
		else
		{
			UE_LOG(LogSubsystemSteam, Warning, TEXT("GetResolvedConnectString Failed"));
		}
	}
	else
	{
		UE_LOG(LogSubsystemSteam, Warning, TEXT("%hs Called, Not Ready to ClientTravel"), __FUNCTION__);
	}
}
void USubsystemSteamManager::StartMatch()
{
	GetWorld()->ServerTravel(TEXT("/Game/Maps/MatchMap?listen"));
}

FNamedOnlineSession* USubsystemSteamManager::GetNamedOnlineSession(FName SessionName)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		return SessionInterface->GetNamedSession(SessionName);
	}
	return nullptr;
}

void USubsystemSteamManager::DebugSessionInfo(FName SessionName)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		LOG_DARK_YELLOW(TEXT("%hs Called, Invalid SessionInterface"), __FUNCTION__);
		return;
	}

	if (FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName))
	{
		FString Str;
		Str += FString::Printf(TEXT("Valid NamedOnlineSession, [SessionName %s]  "), *NamedSession->SessionName.ToString());

		Str += TEXT("[SessionId ");
		if (NamedSession->SessionInfo.IsValid() && NamedSession->SessionInfo->GetSessionId().IsValid())
		{
			Str += NamedSession->SessionInfo->GetSessionId().ToString();
		}
		else
		{
			Str += TEXT("Invalid");
		}
		Str += TEXT("]  ");

		Str += TEXT("[OwningUserId ");
		if (NamedSession->OwningUserId.IsValid())
		{
			Str += NamedSession->OwningUserId->ToString();
		}
		else
		{
			Str += TEXT("Invalid");
		}
		Str += TEXT("]  ");

		Str += TEXT("[LocalOwnerId ");
		if (NamedSession->LocalOwnerId.IsValid())
		{
			Str += NamedSession->LocalOwnerId->ToString();
		}
		else
		{
			Str += TEXT("Invalid");
		}
		Str += TEXT("]  ");
		
		Str += FString::Printf(TEXT("RegisteredPlayers : [RegisteredPlayers.Num() %d, "), NamedSession->RegisteredPlayers.Num());
		for (int32 Index = 0; Index < NamedSession->RegisteredPlayers.Num(); ++Index)
		{
			Str += FString::Printf(TEXT("%d : "), Index);
			const FUniqueNetIdRef IdRef = NamedSession->RegisteredPlayers[Index];
			
			if (IdRef->IsValid())
			{
				Str += IdRef->ToString() + TEXT(", ");
			}
			else
			{
				Str += TEXT("Invalid");
			}
		}
		Str += TEXT("]  ");

		LOG_DARK_YELLOW(TEXT("%s"), *Str);
		return;
	}

	LOG_DARK_YELLOW(TEXT("%hs Called, Invalid NamedOnlineSession"), __FUNCTION__);
}