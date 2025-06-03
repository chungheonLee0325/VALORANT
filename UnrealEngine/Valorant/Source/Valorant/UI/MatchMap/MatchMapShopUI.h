// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Valorant/Player/Component/ShopComponent.h"
#include "MatchMapShopUI.generated.h"

class AAgentPlayerController;
class UTextBlock;
class UImage;

// 구매 결과 이벤트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPurchaseResult, bool, bSuccess, int32, ItemID, EShopItemType, ItemType);

UENUM(BlueprintType)
enum class EAbilityVisualState : uint8
{
	Empty       UMETA(DisplayName = "Empty"),       // 스택 없음 (회색)
	Partial     UMETA(DisplayName = "Partial"),     // 일부 보유 (테두리 강조)
	Full        UMETA(DisplayName = "Full")         // 최대 보유 (밝은 색)
};

// 어빌리티 정보를 담는 구조체
USTRUCT(BlueprintType)
struct FUIAbilityInfo
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 AbilityID = 0;
	
	UPROPERTY(BlueprintReadOnly)
	FString AbilityName = "";
	
	UPROPERTY(BlueprintReadOnly)
	int32 ChargeCost = 0;
	
	UPROPERTY(BlueprintReadOnly)
	UTexture2D* AbilityIcon = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStack = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 MaxStack = 0;
};

/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapShopUI : public UUserWidget
{
	GENERATED_BODY()

public:
	// UI 초기화 함수 - 컨트롤러 설정
	UFUNCTION(BlueprintCallable)
	void InitializeShopUI(AAgentPlayerController* Controller);

	// 크레딧 업데이트 함수
	UFUNCTION(BlueprintCallable)
	void UpdateCreditDisplay(int32 CurrentCredit);

	// 서버에서 최신 크레딧 정보 요청
	UFUNCTION(BlueprintCallable)
	void RequestLatestCreditValue();

	// 상점 아이템 리스트 업데이트
	UFUNCTION(BlueprintCallable)
	void UpdateShopItemList();

	// 특정 카테고리 아이템 리스트 업데이트
	UFUNCTION(BlueprintCallable)
	void UpdateShopItemListByType(EShopItemType ItemType);

	// 구매 결과 이벤트 디스패처
	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnPurchaseResult OnPurchaseResult;

	// 버튼 클릭 이벤트 핸들러
	UFUNCTION(BlueprintCallable)
	void OnClickedBuyWeaponButton(const int WeaponId);

	UFUNCTION(BlueprintCallable)
	void OnClickedBuySkillButton(const int SkillId);

	UFUNCTION(BlueprintCallable)
	void OnClickedBuyShiledButton(const int ShieldId);

	// 무기 목록 갱신 및 보유 중인 무기 하이라이트
	UFUNCTION(Client, Reliable)
	void Client_UpdateWeaponHighlight();
	
	UFUNCTION(BlueprintCallable)
	void UpdateWeaponHighlights();

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponHighlightUpdated();

	// 무기 하이라이트 상태 조회 (Blueprint용)
	UFUNCTION(BlueprintPure, Category = "Shop")
	bool IsWeaponEquipped(int32 WeaponID) const;

	// 무기 하이라이트 색상 가져오기 (Blueprint용)
	UFUNCTION(BlueprintPure, Category = "Shop")
	FLinearColor GetEquippedWeaponHighlightColor() const { return EquippedWeaponHighlightColor; }

	// 어빌리티 스택 관련 함수들
	UFUNCTION(BlueprintPure, Category = "Shop|Ability")
	int32 GetAbilityStack(int32 AbilityID) const;

	UFUNCTION(BlueprintPure, Category = "Shop|Ability")
	int32 GetMaxAbilityStack(int32 AbilityID) const;

	// 어빌리티 목록 갱신 및 스택 표시 업데이트
	UFUNCTION(BlueprintCallable, Category = "Shop|Ability")
	void UpdateAbilityStacks();

	// 어빌리티 스택 변경 이벤트 처리
	UFUNCTION()
	void HandleAbilityStackChanged(int32 AbilityID, int32 NewStack);

	// 어빌리티 스택 변경 시 호출될 블루프린트 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop|Ability")
	void OnAbilityStackChanged(int32 AbilityID, int32 NewStack);

	// 어빌리티 초기화 함수 (InitBySkillData 기반)
	UFUNCTION(BlueprintCallable, Category = "Shop|Ability")
	void InitializeAbilityUI();
	
	// 어빌리티 정보 가져오기 (BlueprintPure로 블루프린트에서 접근 가능)
	UFUNCTION(BlueprintPure, Category = "Shop|Ability")
	FUIAbilityInfo GetAbilityInfo(int32 AbilityID) const;
	
	// 슬롯별 어빌리티 정보 가져오기
	UFUNCTION(BlueprintPure, Category = "Shop|Ability")
	FUIAbilityInfo GetAbilityInfoBySlot(EAbilitySlotType SlotType) const;

	// 어빌리티 정보 업데이트 시 호출될 블루프린트 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop|Ability")
	void OnAbilityInfoUpdated(EAbilitySlotType SlotType, const FUIAbilityInfo& AbilityInfo);

	// 어빌리티 비주얼 상태 가져오기
	UFUNCTION(BlueprintPure, Category = "Shop|Ability")
	EAbilityVisualState GetAbilityVisualState(int32 AbilityID) const;
    
	// 슬롯별 어빌리티 비주얼 상태 가져오기
	UFUNCTION(BlueprintPure, Category = "Shop|Ability")
	EAbilityVisualState GetAbilityVisualStateBySlot(EAbilitySlotType SlotType) const;

	// Blueprint에서 스택 UI를 동적으로 생성/업데이트할 때 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop|Ability")
	void UpdateAbilityStackVisual(EAbilitySlotType SlotType, int32 CurrentStack, int32 MaxStack, EAbilityVisualState VisualState);
protected:
	// 컨트롤러 참조
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	AAgentPlayerController* OwnerController;

	// 크레딧 텍스트 UI
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* CreditText;

	// 어빌리티 이름 UI 
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* AbilityC_Text;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* AbilityQ_Text;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* AbilityE_Text;
	
	// 어빌리티 이미지 UI 
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* AbilityC_Image;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* AbilityQ_Image;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* AbilityE_Image;
	
	// 어빌리티 가격 UI 
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* AbilityC_Cost;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* AbilityQ_Cost;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* AbilityE_Cost;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* C_Stack_1;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* C_Stack_2;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* C_Stack_3;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* Q_Stack_1;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* Q_Stack_2;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* Q_Stack_3;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* E_Stack_1;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* E_Stack_2;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UImage* E_Stack_3;
	
	// 현재 크레딧 값
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 CurrentCredits = 0;

	// 아이템 구매 결과 처리
	UFUNCTION()
	void HandleServerPurchaseResult(bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason);

	// 위젯 초기화 시 호출
	virtual void NativeConstruct() override;

	// 보유 중인 무기 ID 배열
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TArray<int32> EquippedWeaponIDs;
    
	// 무기 하이라이트 스타일
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shop|Style")
	FLinearColor EquippedWeaponHighlightColor = FLinearColor(0.0f, 0.5f, 0.5f, 1.0f);

	// 현재 보유 중인 어빌리티 스택 정보
	UPROPERTY(BlueprintReadOnly, Category = "Shop|Ability")
	TMap<int32, int32> AbilityStacksCache;
	
	// 어빌리티 정보 캐시
	UPROPERTY(BlueprintReadOnly, Category = "Shop|Ability")
	TMap<int32, FUIAbilityInfo> AbilityInfoCache;

	UPROPERTY(BlueprintReadWrite, Category = "Shop|Ability")
	int SkillQID = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Shop|Ability")
	int SkillEID = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Shop|Ability")
	int SkillCID = 0;
	
	// 어빌리티 데이터 로드
	void LoadAbilityData(int32 AbilityID);
	
	// 모든 어빌리티 데이터 로드
	void LoadAllAbilityData();
	
	// 어빌리티 UI 업데이트
	void UpdateAbilityUI();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shop|Ability")
	FColor DefaultColor = FColor(155,155,255);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shop|Ability")
	FColor PurchaseColor = FColor(0,255,156);

private:
	UPROPERTY()
	UValorantGameInstance* GameInstance;

	// 텍스트 색상 변경을 위한 타이머 핸들
	FTimerHandle CreditTextColorTimerHandle;

	
	// 텍스트 원래 색상 캐싱
	FSlateColor OriginalCreditTextColor;
	
	// 색상 변경 함수
	void FlashCreditTextColor(const FLinearColor& FlashColor, float Duration = 0.5f);
	
	// 원래 색상으로 복원하는 함수
	void ResetCreditTextColor();
};
