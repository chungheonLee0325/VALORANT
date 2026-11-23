#pragma once

#include "MVVMViewModelBase.h"
#include "GameManager/MatchGameMode.h"
#include "Player/Component/ShopComponent.h"
#include "TimerManager.h"
#include "ShopViewModel.generated.h"

class AAgentPlayerState;
class ABaseAgent;
class AMatchGameState;
class UBaseAttributeSet;

struct FShopPurchaseOutcome
{
	bool bSuccess;
	int32 ItemID;
	EShopItemType ItemType;
	FString FailureReason;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FShopAbilityStackUpdated, int32, int32);

UCLASS()
class UShopViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Bind(AAgentPlayerController& Controller);
	void Unbind();
	void BeginSession();
	void EndSession();
	void SelectItem(const FShopItem& Item);
	bool RequestPurchase();
	TArray<FShopItem> GetItems(EShopItemType Type) const;
	TOptional<FShopPurchaseOutcome> TakePurchaseOutcome();

	FShopAbilityStackUpdated OnAbilityStackUpdated;
	FSimpleMulticastDelegate OnEquipmentUpdated;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	int32 CurrentCredits = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	int32 SelectedItemId = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	FString SelectedItemName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	int32 SelectedItemPrice = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	bool bCanPurchase = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	bool bPurchasePending = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Shop")
	FString PurchaseResultText;

private:
	void UnbindSources();
	void ResetSessionState();
	void OnPurchaseTimeout();
	void OnEquipmentChanged();
	void OnGameStateSet(AGameStateBase* State);

	UFUNCTION()
	void OnCreditChanged(int32 Credits);

	UFUNCTION()
	void OnAbilityStackChanged(int32 AbilityID, int32 NewStack);

	UFUNCTION()
	void OnRoundChanged(ERoundSubState State, float TransitionTime);

	void OnPurchaseResult(uint32 RequestID, bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason);

	UFUNCTION()
	void OnArmorChanged(float Armor);

	UFUNCTION()
	void RefreshAvailability();

	TWeakObjectPtr<AAgentPlayerController> Controller;
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<AAgentPlayerState> PlayerState;
	TWeakObjectPtr<AMatchGameState> GameState;
	TWeakObjectPtr<ABaseAgent> Agent;
	TWeakObjectPtr<UBaseAttributeSet> Attributes;
	EShopItemType SelectedItemType = EShopItemType::None;
	EShopItemType PendingItemType = EShopItemType::None;
	int32 PendingItemId = 0;
	uint32 PendingRequestID = 0;
	bool bSessionOpen = false;
	bool bReportPendingResult = false;
	FTimerHandle PurchaseResponseTimer;
	TOptional<FShopPurchaseOutcome> PurchaseOutcome;
};
