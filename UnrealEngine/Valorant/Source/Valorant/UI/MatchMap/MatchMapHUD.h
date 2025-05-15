// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameManager/MatchGameState.h"
#include "ResourceManager/ValorantGameType.h"
#include "MatchMapHUD.generated.h"

class UAgentAbilitySystemComponent;
class AAgentPlayerState;
class UWidgetSwitcher;
class UTextBlock;
class UValorantGameInstance;

UENUM(BlueprintType)
enum class EMatchAnnouncement : uint8
{
	EMA_Won,
	EMA_Lost,
	EMA_BuyPhase,
	EMA_SpikeActivated_Won,   // 스파이크 폭발로 인한 승리
	EMA_SpikeActivated_Lost,  // 스파이크 폭발로 인한 패배
	EMA_SpikeDefused_Won,     // 스파이크 해제로 인한 승리
	EMA_SpikeDefused_Lost     // 스파이크 해제로 인한 패배
};

// 스킬 슬롯별 스택 정보 구조체
USTRUCT(BlueprintType)
struct FAbilitySlotStackInfo
{
	GENERATED_BODY()

	// 현재 스택 수
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStack = 0;

	// 최대 스택 수
	UPROPERTY(BlueprintReadOnly)
	int32 MaxStack = 0;

	// 어빌리티 ID
	UPROPERTY(BlueprintReadOnly)
	int32 AbilityID = 0;

	// 초기화 함수
	void Initialize(int32 InAbilityID, int32 InCurrentStack, int32 InMaxStack)
	{
		AbilityID = InAbilityID;
		CurrentStack = InCurrentStack;
		MaxStack = InMaxStack;
	}
};

/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapHUD : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess))
	bool bPlayed60SecLeftVo = true;
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess))
	bool bPlayed30SecLeftVo = true;
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess))
	bool bPlayed10SecLeftVo = true;
	void SetTrueVo();
	void SetFalseVo();
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void UpdateTime(float Time);
	UFUNCTION()
	void UpdateScore(int TeamBlueScore, int TeamRedScore);
	UFUNCTION()
	void OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime);
	UFUNCTION()
	void OnRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason, const float TransitionTime);
	UFUNCTION(BlueprintImplementableEvent)
	void PlayRoundEndVFX(bool bWin);
	UFUNCTION(BlueprintImplementableEvent)
	void PlayRemTimeVO(const int Level);

	FTimerHandle AnnouncementTimerHandle;
	UFUNCTION()
	void DisplayAnnouncement(EMatchAnnouncement MatchAnnouncement, const float DisplayTime);
	void HideAnnouncement();

	UFUNCTION()
	void UpdateDisplayHealth(const float health);
	UFUNCTION()
	void UpdateDisplayArmor(const float armor);

	// 어빌리티 스택 관련 함수들
	UFUNCTION()
	void HandleAbilityStackChanged(int32 AbilityID, int32 NewStack);

	// 어빌리티 슬롯 정보 초기화 메서드(어빌리티 max 갯수)
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Stack")
	void InitializeAbilityMaxStacks(EAbilitySlotType AbilitySlot, int32 MaxStacks);

	// 전체 슬롯 스택 정보 업데이트 (블루프린트용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Stack")
	void OnSlotStackInfoUpdated(EAbilitySlotType AbilitySlot, const FAbilitySlotStackInfo& StackInfo);

	// 어빌리티 스택 초기화
	UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
	void InitializeAbilityStacks();

	// 특정 슬롯의 스택 정보 업데이트
	UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
	void UpdateSlotStackInfo(EAbilitySlotType AbilitySlot, int32 AbilityID);
	FAbilitySlotStackInfo GetSlotStackInfo(EAbilitySlotType AbilitySlot) const;

public:
	UFUNCTION(BlueprintCallable)
	void BindToDelegatePC(UAgentAbilitySystemComponent* asc, AAgentPlayerController* pc);

	UFUNCTION(BlueprintCallable)
	void InitUI(AAgentPlayerState* ps);

	// 어빌리티 스택 정보 가져오기 함수
	UFUNCTION(BlueprintPure, Category = "Ability|Stack")
	int32 GetAbilityStack(int32 AbilityID) const;

	UFUNCTION(BlueprintPure, Category = "Ability|Stack")
	int32 GetMaxAbilityStack(int32 AbilityID) const;

	// 스킬 ID 가져오기 함수
	UFUNCTION(BlueprintPure, Category = "Ability|Stack")
	int32 GetSkillCID() const { return SkillCID; }

	UFUNCTION(BlueprintPure, Category = "Ability|Stack")
	int32 GetSkillQID() const { return SkillQID; }

	UFUNCTION(BlueprintPure, Category = "Ability|Stack")
	int32 GetSkillEID() const { return SkillEID; }
	
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockTime = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockBlueScore = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRedScore = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcherAnnouncement = nullptr;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_Armor;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_HP;

private:
	UPROPERTY()
	UAgentAbilitySystemComponent* ASC;
	
	// 어빌리티 스택 정보 캐시
	UPROPERTY()
	TMap<int32, int32> AbilityStacksCache;

	// 현재 에이전트 스킬 ID
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillCID = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillQID = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillEID = 0;

	// 게임 인스턴스 참조
	UPROPERTY()
	UValorantGameInstance* GameInstance;
	
/*
 *	Debug
 */
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRoundSubStateDbg = nullptr;

protected:
	UFUNCTION()
	void DebugRoundSubState(const FString& RoundSubStateStr);

	// 슬롯별 스택 정보
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	TMap<EAbilitySlotType, FAbilitySlotStackInfo> SlotStackInfoMap;
};
