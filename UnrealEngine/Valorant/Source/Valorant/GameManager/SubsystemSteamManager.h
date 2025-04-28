// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SubsystemSteamManager.generated.h"

// Custom Log Category
DECLARE_LOG_CATEGORY_EXTERN(LogSubsystemSteam, Log, All);

// Custom Log using dark yellow color
#define LOG_DARK_YELLOW(Format, ...) \
SET_WARN_COLOR(COLOR_DARK_YELLOW) \
UE_LOG(LogSubsystemSteam, Log, TEXT("%s"), *FString::Printf(Format, ##__VA_ARGS__)) \
CLEAR_WARN_COLOR()

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateSteamSessionComplete, bool /* bWasSuccessful */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFindSteamSessionsComplete, const TArray<FOnlineSessionSearchResult>& /* SessionResults */, bool /* bWasSuccessful */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFindFirstSteamSessionComplete, const FOnlineSessionSearchResult& /* SessionResult */, bool /* bWasSuccessful */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnJoinSteamSessionComplete, EOnJoinSessionCompleteResult::Type /* Result */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDestroySteamSessionComplete, bool /* bWasSuccessful */);

/**
 * 게임이 시작되면 자동으로 GameInstance에 붙는다.
 */
UCLASS()
class VALORANT_API USubsystemSteamManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USubsystemSteamManager();

	/*
	 * Get Online Subsystem Interface
	 */
	static IOnlineSessionPtr GetSessionInterface();

	/*
	 * To handle session functionality.
	 */
	void CreateSession();
	void FindSessions();
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();

	static FString GetDisplayName(int UniquePlayerID = 0);
	
	// SessionName에 해당하는 NamedOnlineSession 반환
	static FNamedOnlineSession* GetNamedOnlineSession(FName SessionName = NAME_GameSession);

	// SessionName에 해당하는 Session의 정보를 FString으로 합쳐 반환
	static void DebugSessionInfo(FName SessionName = NAME_GameSession);

	/*
	 * Delegate
	 */
	FOnCreateSteamSessionComplete OnCreateSteamSessionComplete;
	FOnFindSteamSessionsComplete OnFindSteamSessionsComplete;
	FOnFindFirstSteamSessionComplete OnFindFirstSteamSessionComplete;
	FOnJoinSteamSessionComplete OnJoinSteamSessionComplete;
	FOnDestroySteamSessionComplete OnDestroySteamSessionComplete;

	/*
	 * TimerHandle
	 */
	FTimerHandle CheckSessionHandle;
	
	int ReqMatchAutoStartPlayerCount = 2;

private:
	void CheckHostingSession();
	void CheckJoinSession();
	void StartMatch();
	
	/*
	* Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	* This don't need to be called outside this class.
	*/
	void OnCreateSessionComplete_Internal(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete_Internal(bool bWasSuccessful);
	void OnJoinSessionComplete_Internal(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete_Internal(FName SessionName, bool bWasSuccessful);

	// 가장 최근에 생성된 Session 설정
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	// 가장 최근의 세션 검색 정보
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	/*
	 *	To add to the Online Session Interface delegate list.
	 *	We'll bind our MultiplayerSessionsSubsystem internal callbacks to these.
	 */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;

	// 세션을 생성할 때 이미 세션이 존재하면 그 세션 제거가 완료될 때 다시 세션 생성하기 위한 트리거
	bool bCreateSessionOnDestroy = false;

	const FName NAME_Identifier = TEXT("Identifier");
	const FString SessionIdentifier = TEXT("VALORITHM");
};
