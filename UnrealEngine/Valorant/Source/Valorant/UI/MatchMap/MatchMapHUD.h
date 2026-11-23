// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatHUDViewModel.h"
#include "HUDAbilityBarModel.h"
#include "GameManager/MatchGameState.h"
#include "Player/Animaiton/AgentAnimInstance.h"
#include "ResourceManager/ValorantGameType.h"
#include "MatchMapHUD.generated.h"

class UMatchMapHudTopWidget;
class UVerticalBox;
struct FKillFeedInfo;
enum class EFollowUpInputType : uint8;
class UOverlay;
class UHorizontalBox;
class UAgentAbilitySystemComponent;
class AAgentPlayerState;
class UWidgetSwitcher;
class UTextBlock;
class UImage;
class UPanelWidget;
class UCanvasPanel;

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

	// 라운드 시작 3초전 카운트다운 음향을 위한 변수
	bool bIsPreRound = false;

	UFUNCTION(BlueprintCallable)
	void SetTrueVo();
	void SetFalseVo();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void UpdateTime(float Time);
	
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
	void OnDamaged(const FVector& HitOrg, const EAgentDamagedPart DamagedPart, const EAgentDamagedDirection DamagedDirection, const bool bDie, const bool bLarge, const bool bLowState);
	UFUNCTION(BlueprintImplementableEvent)
	void OnHealed(const bool bHighState);
	
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

public:
	void RefreshPlayerSources();

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
	
	// 어빌리티 상세 정보 가져오기
	UFUNCTION(BlueprintPure, Category = "Ability|Info")
	FHUDAbilityInfo GetAbilityInfo(int32 AbilityID) const;
	
	// 슬롯별 어빌리티 상세 정보 가져오기
	UFUNCTION(BlueprintPure, Category = "Ability|Info")
	FHUDAbilityInfo GetAbilityInfoBySlot(EAbilitySlotType SlotType) const;
	
	// 어빌리티 정보 업데이트 이벤트 (블루프린트에서 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Info")
	void OnAbilityInfoUpdated(EAbilitySlotType SlotType, const FHUDAbilityInfo& AbilityInfo);

	UFUNCTION(BlueprintImplementableEvent)
	void OnMatchEnd(bool bWin);

	UFUNCTION(BlueprintImplementableEvent)
	void SpikePlanted();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnDisplayIndicator(const FVector& HitOrg);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLowState(bool bCond);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnSwitchWeapon(EInteractorType interactorType);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpikeOwnChanged(bool bOwnSpike);

	UFUNCTION()
	void OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& KillFeedInfo);
	UFUNCTION(BlueprintImplementableEvent)
	void DisplayKillFeed(const UTexture2D* InstigatorIcon, const FString& InstigatorName, const bool bInstigatorIsMyTeam,
		const UTexture2D* VictimIcon, const FString& VictimName, const bool bVictimIsMyTeam,
		const FKillFeedInfo& KillFeedReason, const UTexture2D* ReasonIcon);
	
	// 후속 입력 스킬 발동시, 후속 입력키 UI를 표시
	UFUNCTION()
	void DisplayFollowUpInputUI(FGameplayTag slotTag, EFollowUpInputType inputType);
	
	// 후속 입력 스킬 종료시, 후속 입력키 UI를 숨김
	UFUNCTION()
	void HideFollowUpInputUI();

	UFUNCTION(BlueprintImplementableEvent)
	void OnSuppressed(FGameplayTag GameplayTag, int num);

	// 스파이크 설치 및 해제
	UFUNCTION(BlueprintImplementableEvent)
	void DisplaySpikeProgress();
	UFUNCTION(BlueprintImplementableEvent)
	void HideSpikeProgress();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpikeProgressBarUpdate(const float ratio);
	UFUNCTION(BlueprintImplementableEvent)
	void ResetSpikeProgressBar();
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetSpikeProgressTextToPlant();
	UFUNCTION(BlueprintImplementableEvent)
	void SetSpikeProgressTextToDefuse();

	// 녹음 시작
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void StartRecording();
	
	// 녹음 종료
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void EndRecording();
	
	// ai 답변 받아서 업데이트
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateAIAnswer(const FString& answer);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ResetRecordUI();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateCreditDisplay(int32 NewCredit);
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcherAnnouncement = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> HorizontalBoxAmmo = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockMagazineAmmo = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockSpareAmmo = nullptr;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UMatchMapHudTopWidget> TopWidget = nullptr;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_Armor;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* txt_HP;
	
	// 어빌리티 이미지
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "Ability|UI")
	UImage* AbilityC_Image;
	
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "Ability|UI")
	UImage* AbilityQ_Image;
	
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "Ability|UI")
	UImage* AbilityE_Image;
	
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "Ability|UI")
	UImage* AbilityX_Image;

	// Names match the existing HUD asset. BindWidget validates these types during Blueprint compilation.
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "C Key Label"))
	TObjectPtr<UTextBlock> TextBlock_101;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "C Stack Panel"))
	TObjectPtr<UCanvasPanel> CanvasPanel_1;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "Q Key Label"))
	TObjectPtr<UTextBlock> TextBlock;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "Q Stack Panel"))
	TObjectPtr<UCanvasPanel> CanvasPanel_0;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "E Key Label"))
	TObjectPtr<UTextBlock> TextBlock_1;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "E Stack Panel"))
	TObjectPtr<UCanvasPanel> CanvasPanel;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "X Key Label"))
	TObjectPtr<UTextBlock> TextBlock_2;
	UPROPERTY(BlueprintReadOnly, Category = "Ability|UI", meta = (BindWidget, DisplayName = "X Stack Panel"))
	TObjectPtr<UCanvasPanel> CanvasPanel_2;

	// 후속 입력키 UI
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	UOverlay* Left_E;
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	UVerticalBox* LeftOrRight_E;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	UOverlay* Left_Q;
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	UVerticalBox* LeftOrRight_Q;
	
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	UOverlay* Left_C;
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	UVerticalBox* LeftOrRight_C;
	
	// UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	// UOverlay* Left_X;
	// UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = "FollowUpInput")
	// UHorizontalBox* LeftOrRight_X;

private:
    void BindMatchState(AGameStateBase* State);
    void UnbindPlayer();
    void CacheAbilityWidgets();
    void RestoreAbilityWidgets();
    void RenderAbilitySlot(EAbilitySlotType AbilitySlot, bool bLayoutChanged);
    UImage* GetAbilityImage(EAbilitySlotType AbilitySlot) const;
    void OnAbilitySlotChanged(EAbilitySlotType AbilitySlot);
    void OnStatusFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field);
    void RenderVitals();
    void RenderAmmo();

    UFUNCTION()
    void OnPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCombatHUDViewModel> CombatViewModel;

    UPROPERTY(Transient)
    TObjectPtr<UHUDAbilityBarModel> AbilityBar;

    UPROPERTY(EditDefaultsOnly, Category = "HUD|Style")
    FLinearColor SecondaryTextColor = FLinearColor(0.62f, 0.90f, 0.91f);

    UPROPERTY(EditDefaultsOnly, Category = "HUD|Style")
    FLinearColor WarningTextColor = FLinearColor(1.0f, 0.67f, 0.36f);

    UPROPERTY(EditDefaultsOnly, Category = "HUD|Style")
    FLinearColor UnavailableSkillColor = FLinearColor(0.68f, 0.71f, 0.76f);

    UPROPERTY(EditDefaultsOnly, Category = "HUD|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UnavailableSkillOpacity = 0.30f;

    UPROPERTY(EditDefaultsOnly, Category = "HUD|Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowHealthFraction = 0.25f;

    UPROPERTY(Transient)
    TMap<EAbilitySlotType, FSlateBrush> AuthoredAbilityBrushes;

    TMap<EAbilitySlotType, TWeakObjectPtr<UTextBlock>> AbilityKeyLabels;
    TMap<EAbilitySlotType, TWeakObjectPtr<UWidget>> AbilityPipPanels;
    TMap<EAbilitySlotType, FText> AuthoredAbilityKeys;

    TWeakObjectPtr<AAgentPlayerController> BoundController;
    TWeakObjectPtr<AAgentPlayerState> BoundPlayerState;
    TWeakObjectPtr<AMatchGameState> BoundMatchState;
    TWeakObjectPtr<UAgentAbilitySystemComponent> BoundASC;
    FDelegateHandle SuppressionHandle;
    bool bHudConstructed = false;

	// 현재 에이전트 스킬 ID
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillCID = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillQID = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillEID = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	int32 SkillXID = 0;

	// 활성화된 후속 입력키 UI 다시 숨기기
	UPROPERTY()
	UPanelWidget* DisplayedFollowUpInputUI = nullptr;
	
/*
 *	Debug
 */
public:
	// UPROPERTY(meta=(BindWidget))
	// TObjectPtr<UTextBlock> TextBlockRoundSubStateDbg = nullptr;

protected:
	UFUNCTION()
	void DebugRoundSubState(const FString& RoundSubStateStr);

	// 슬롯별 스택 정보
	UPROPERTY(BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	TMap<EAbilitySlotType, FAbilitySlotStackInfo> SlotStackInfoMap;
};
