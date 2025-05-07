// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Valorant/Player/Component/ShopComponent.h"
#include "MatchMapShopUI.generated.h"

class AAgentPlayerController;

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

protected:
	// 컨트롤러 참조
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	AAgentPlayerController* OwnerController;

	// 아이템 구매 결과 처리 함수
	UFUNCTION(BlueprintCallable)
	void HandlePurchaseResult(bool bSuccess, int32 ItemID, EShopItemType ItemType);
};
