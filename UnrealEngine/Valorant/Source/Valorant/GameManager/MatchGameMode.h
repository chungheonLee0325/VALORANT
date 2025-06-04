// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Web/DatabaseManager.h"
#include "MatchGameMode.generated.h"

struct FMatchDTO;
struct FPlayerMatchDTO;
struct FPlayerDTO;
class AAgentPlayerState;
class ASpike;
class ABaseWeapon;
class ABaseAgent;
class AMatchPlayerController;
class AAgentPlayerController;
class APlayerStart;
class USubsystemSteamManager;
class UValorantGameInstance;

// MatchState가 InProgress일 때 SubState 
UENUM(BlueprintType)
enum class ERoundSubState : uint8
{
	RSS_None,
	RSS_SelectAgent,
	RSS_PreRound,
	RSS_BuyPhase,
	RSS_InRound,
	RSS_EndPhase
};

UENUM(BlueprintType)
enum class ERoundEndReason : uint8
{
	ERER_None,
	ERER_Eliminated,
	ERER_Timeout,
	ERER_SpikeActive,
	ERER_SpikeDefuse
};

USTRUCT(BlueprintType)
struct FMatchPlayer
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AAgentPlayerController> Controller = nullptr;
	FString Nickname = "UNKNOWN";
	bool bIsBlueTeam = true;
	bool bIsDead = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartInRound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartPreRound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndRound);

UCLASS()
class VALORANT_API AMatchGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMatchGameMode();
	bool IsAttacker(const bool bIsBlueTeam) const;
	bool IsShifted() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	UPROPERTY()
	TObjectPtr<UValorantGameInstance> ValorantGameInstance = nullptr;
	UPROPERTY()
	TObjectPtr<USubsystemSteamManager> SubsystemManager = nullptr;

	// 플레이어 목록, 매치가 종료되면 전적 갱신을 위해 들고있는다
	UPROPERTY()
	TMap<AAgentPlayerController*, FPlayerDTO> PlayerMap;
	// 현재 매치 정보
	TSharedPtr<FMatchDTO> CurrentMatchInfo;
	// 현재 매치에서 플레이어의 기록 저장
	UPROPERTY()
	TMap<AAgentPlayerController*, FPlayerMatchDTO> PlayerMatchInfoMap;
	FOnPostMatchCompleted OnPostMatchCompletedDelegate;

	UFUNCTION()
	void OnPostMatchCompleted(const bool bIsSuccess, const FMatchDTO& CreatedMatchDto);
	
	UPROPERTY(Transient)
	ERoundSubState RoundSubState = ERoundSubState::RSS_None;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int RequiredPlayerCount = 9999;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	int LoggedInPlayerNum = 0;
	UPROPERTY(BlueprintReadOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TArray<FMatchPlayer> MatchPlayers;
	
	int LockedInPlayerNum = 0;
	TArray<FString> RedTeamPlayerNameArray;
	TArray<FString> BlueTeamPlayerNameArray;
	UPROPERTY()
	TObjectPtr<APlayerStart> AttackersStartPoint = nullptr;
	UPROPERTY()
	TObjectPtr<APlayerStart> DefendersStartPoint = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSubclassOf<ABaseAgent> AgentClass;
	UPROPERTY(EditDefaultsOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSubclassOf<ABaseWeapon> MeleeAsset;
	UPROPERTY(EditDefaultsOnly, Category="Gameflow", meta=(AllowPrivateAccess))
	TSubclassOf<ABaseWeapon> ClassicAsset;
	
	UPROPERTY()
	ASpike* Spike;

public:
	void OnControllerBeginPlay(AMatchPlayerController* Controller, const FString& Nickname, const FString& RealNickname);
	void OnLockIn(AMatchPlayerController* Player, int AgentId);
	void OnAgentSelected(AMatchPlayerController* MatchPlayerController, int SelectedAgentID);
	
	// 구매 페이즈 종료 시 모든 무기를 사용됨으로 표시
	UFUNCTION(BlueprintCallable, Category="Round")
	void MarkAllWeaponsAsUsed();

	// 현재 라운드 상태 가져오기
	UFUNCTION(BlueprintCallable, Category="Round")
	ERoundSubState GetRoundSubState() const;

	// 현재 상점을 열 수 있는 상태인지 확인
	UFUNCTION(BlueprintCallable, Category="Shop")
	bool CanOpenShop() const;

protected:
	FTimerHandle RoundTimerHandle;

	float MaxTime = 0.0f;
	float RemainRoundStateTime = 0.0f;
	float SelectAgentTime = 60.0f;
	float PreRoundTime = 35.0f;		// org: 45.0f
	float BuyPhaseTime = 20.0f;		// org: 30.0f
	float InRoundTime = 100.0f;		// org: 100.0f
	float EndPhaseTime = 7.0f;		// org: 10.0f
	float SpikeActiveTime = 45.0f;	// org: 45.0f
	bool bReadyToEndMatch = false;
	float LeavingMatchTime = 10.0f;
	
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual bool ReadyToEndMatch_Implementation() override;
	void LeavingMatch();
	virtual void HandleMatchHasEnded() override;
	void StartSelectAgent();
	void StartPreRound();
	void StartBuyPhase();
	void StartInRound();
	void StartEndPhaseByTimeout();
	void StartEndPhaseByEliminated(const bool bBlueWin);
	void StartEndPhaseBySpikeActive();
	void StartEndPhaseBySpikeDefuse();
	void HandleRoundSubState_SelectAgent();
	void HandleRoundSubState_PreRound();
	void HandleRoundSubState_BuyPhase();
	void HandleRoundSubState_InRound();
	void HandleRoundSubState_EndPhase();
	void SetRoundSubState(ERoundSubState NewRoundSubState);

public:
	int TotalRound = 3;
	int CurrentRound = 0;
	int RequiredScore = 2;
	int TeamBlueScore = 0;
	int TeamRedScore = 0;
	int ShiftRound = 2;
	int BlueTeamConsecutiveLosses = 0;
	int RedTeamConsecutiveLosses = 0;
	void HandleRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason);

	// 크레딧 시스템 관련 함수
	void AwardRoundEndCredits();

	bool bSpikePlanted = false;
	int TeamBlueRemainingAgentNum = 0;
	int TeamRedRemainingAgentNum = 0;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	void ClearObjects();
	void RespawnAll();
	void RespawnPlayer(AAgentPlayerState* ps, AAgentPlayerController* pc, FTransform spawnTransform);
	void ResetAgentGAS(AAgentPlayerState* AgentPS);
	void OnKill(AAgentPlayerController* Killer, AAgentPlayerController* Victim);
	void OnDie(AMatchPlayerController* Victim);
	void OnRevive(AMatchPlayerController* Reviver, AMatchPlayerController* Target);
	void OnSpikePlanted(AAgentPlayerController* Planter);
	void OnSpikeDefused(AAgentPlayerController* Defuser);
	
	// 공격팀에게 스파이크 스폰
	void SpawnSpikeForAttackers();
	
	void SpawnDefaultWeapon();
	void SpawnDefaultWeapon(ABaseAgent* agent);

	UFUNCTION()
	void DestroySpikeInWorld();

	UFUNCTION(Category = "Log")
	void SubmitShotLog(AAgentPlayerController* pc, int32 fireCount, int32 hitCount, int32 headshotCount, int damage);


	UPROPERTY(EditAnywhere)
	TSubclassOf<ASpike> SpikeClass;

	FOnStartPreRound OnStartPreRound;
	FOnStartInRound OnStartInRound;
	FOnEndRound OnEndRound;

	UFUNCTION(BlueprintCallable)
	void PrintAllPlayerLogs() const;

	// 각 PC에게 게임 시작 알림 - 로딩 UI 제거용
	UFUNCTION()
	void NotifyGameStart(AMatchPlayerController* PC, bool bDisplay);
};
