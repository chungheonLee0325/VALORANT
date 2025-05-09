// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Valorant/Player/Component/ShopComponent.h"
#include "MatchMapShopUI.generated.h"

class AAgentPlayerController;
class UTextBlock;

// 구매 결과 이벤트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPurchaseResult, bool, bSuccess, int32, ItemID, EShopItemType, ItemType);

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
protected:
	// 컨트롤러 참조
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	AAgentPlayerController* OwnerController;

	// 크레딧 텍스트 UI
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Shop|UI")
	UTextBlock* CreditText;

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
};
