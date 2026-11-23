#include "ShopViewModel.h"

#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "GameManager/MatchGameState.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Player/Component/CreditComponent.h"

namespace
{
	constexpr float PurchaseResponseTimeout = 10.f;
}

void UShopViewModel::Bind(AAgentPlayerController& InController)
{
	if (Controller.Get() != &InController)
	{
		Unbind();
		Controller = &InController;
		InController.OnShopPurchaseResponse.AddUObject(this, &UShopViewModel::OnPurchaseResult);
	}

	auto* world = InController.GetWorld();
	if (BoundWorld.Get() != world)
	{
		if (auto* previous = BoundWorld.Get())
		{
			previous->GameStateSetEvent.RemoveAll(this);
		}
		BoundWorld = world;
		if (world)
		{
			world->GameStateSetEvent.AddUObject(this, &UShopViewModel::OnGameStateSet);
		}
	}
	auto* playerState = InController.GetPlayerState<AAgentPlayerState>();
	auto* gameState = world ? world->GetGameState<AMatchGameState>() : nullptr;
	auto* agent = InController.GetPawn<ABaseAgent>();
	auto* attributes = playerState ? playerState->GetBaseAttributeSet() : nullptr;
	if (PlayerState.Get() != playerState || GameState.Get() != gameState
		|| Agent.Get() != agent || Attributes.Get() != attributes)
	{
		UnbindSources();
		PlayerState = playerState;
		GameState = gameState;
		Agent = agent;
		Attributes = attributes;
		if (playerState)
		{
			if (auto* credits = playerState->GetCreditComponent())
			{
				credits->OnCreditChanged.AddUniqueDynamic(this, &UShopViewModel::OnCreditChanged);
			}
			playerState->OnAbilityStackChanged.AddUniqueDynamic(this, &UShopViewModel::OnAbilityStackChanged);
			playerState->OnShopArmorTierChanged.AddUObject(this, &UShopViewModel::RefreshAvailability);
		}
		if (gameState)
		{
			gameState->OnRoundSubStateChanged.AddUniqueDynamic(this, &UShopViewModel::OnRoundChanged);
		}
		if (agent)
		{
			agent->OnWeaponSlotsChanged.AddUObject(this, &UShopViewModel::OnEquipmentChanged);
		}
		if (attributes)
		{
			attributes->OnArmorChanged.AddUniqueDynamic(this, &UShopViewModel::OnArmorChanged);
		}
	}

	OnCreditChanged(playerState ? playerState->GetCurrentCredit() : 0);
}

void UShopViewModel::UnbindSources()
{
	if (auto* state = PlayerState.Get())
	{
		if (auto* credits = state->GetCreditComponent())
		{
			credits->OnCreditChanged.RemoveDynamic(this, &UShopViewModel::OnCreditChanged);
		}
		state->OnAbilityStackChanged.RemoveDynamic(this, &UShopViewModel::OnAbilityStackChanged);
		state->OnShopArmorTierChanged.RemoveAll(this);
	}
	if (auto* state = GameState.Get())
	{
		state->OnRoundSubStateChanged.RemoveDynamic(this, &UShopViewModel::OnRoundChanged);
	}
	if (auto* agent = Agent.Get())
	{
		agent->OnWeaponSlotsChanged.RemoveAll(this);
	}
	if (auto* attributes = Attributes.Get())
	{
		attributes->OnArmorChanged.RemoveDynamic(this, &UShopViewModel::OnArmorChanged);
	}
	PlayerState.Reset();
	GameState.Reset();
	Agent.Reset();
	Attributes.Reset();
}

void UShopViewModel::Unbind()
{
	UnbindSources();
	if (auto* world = BoundWorld.Get())
	{
		world->GameStateSetEvent.RemoveAll(this);
	}
	BoundWorld.Reset();
	if (auto* pc = Controller.Get())
	{
		pc->OnShopPurchaseResponse.RemoveAll(this);
		pc->GetWorldTimerManager().ClearTimer(PurchaseResponseTimer);
	}
	Controller.Reset();
	EndSession();
	PendingRequestID = 0;
	UE_MVVM_SET_PROPERTY_VALUE(bPurchasePending, false);
}

void UShopViewModel::BeginSession()
{
	bSessionOpen = true;
	ResetSessionState();
}

void UShopViewModel::EndSession()
{
	bSessionOpen = false;
	ResetSessionState();
}

void UShopViewModel::ResetSessionState()
{
	bReportPendingResult = false;
	PurchaseOutcome.Reset();
	UE_MVVM_SET_PROPERTY_VALUE(PurchaseResultText, FString());
	SelectItem(FShopItem());
}

TArray<FShopItem> UShopViewModel::GetItems(EShopItemType Type) const
{
	const auto* shop = Controller.IsValid() ? Controller->ShopComponent : nullptr;
	return IsValid(shop) ? shop->GetShopItemsByType(Type) : TArray<FShopItem>();
}

TOptional<FShopPurchaseOutcome> UShopViewModel::TakePurchaseOutcome()
{
	auto result = MoveTemp(PurchaseOutcome);
	PurchaseOutcome.Reset();
	return result;
}

void UShopViewModel::SelectItem(const FShopItem& Item)
{
	SelectedItemType = Item.ItemType;
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItemId, Item.ItemID);
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItemName, Item.ItemName);
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItemPrice, Item.Price);
	RefreshAvailability();
}

void UShopViewModel::RefreshAvailability()
{
	int32 refund = 0;
	const auto* shop = Controller.IsValid() ? Controller->ShopComponent : nullptr;
	const bool bAvailable = bSessionOpen && !bPurchasePending && IsValid(shop)
		&& shop->CanPurchaseItem(SelectedItemId, SelectedItemType, refund);
	UE_MVVM_SET_PROPERTY_VALUE(bCanPurchase, bAvailable);
}

bool UShopViewModel::RequestPurchase()
{
	RefreshAvailability();
	auto* pc = Controller.Get();
	if (!pc || !bCanPurchase)
	{
		return false;
	}

	PendingItemId = SelectedItemId;
	PendingItemType = SelectedItemType;
	PendingRequestID = pc->AllocateShopRequestID();
	bReportPendingResult = true;
	UE_MVVM_SET_PROPERTY_VALUE(PurchaseResultText, FString());
	UE_MVVM_SET_PROPERTY_VALUE(bPurchasePending, true);
	RefreshAvailability();

	// On a listen host the client RPC can complete synchronously.
	pc->GetWorldTimerManager().SetTimer(PurchaseResponseTimer, this, &UShopViewModel::OnPurchaseTimeout, PurchaseResponseTimeout);
	pc->Server_RequestShopPurchase(PendingItemId, PendingItemType, PendingRequestID);
	return true;
}

void UShopViewModel::OnCreditChanged(int32 Credits)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentCredits, Credits))
	{
		UE_LOG(LogTemp, Display, TEXT("Shop credits: %d"), Credits);
	}
	RefreshAvailability();
}

void UShopViewModel::OnAbilityStackChanged(int32 AbilityID, int32 NewStack)
{
	RefreshAvailability();
	OnAbilityStackUpdated.Broadcast(AbilityID, NewStack);
}

void UShopViewModel::OnRoundChanged(ERoundSubState State, float TransitionTime)
{
	RefreshAvailability();
	if (Controller.IsValid() && GameState.IsValid() && !GameState->CanOpenShop())
	{
		Controller->CloseShopUI();
	}
}

void UShopViewModel::OnEquipmentChanged()
{
	RefreshAvailability();
	OnEquipmentUpdated.Broadcast();
}

void UShopViewModel::OnGameStateSet(AGameStateBase* State)
{
	if (auto* pc = Controller.Get())
	{
		pc->CloseShopUI();
		Bind(*pc);
	}
}

void UShopViewModel::OnArmorChanged(float Armor)
{
	RefreshAvailability();
}

void UShopViewModel::OnPurchaseResult(uint32 RequestID, bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason)
{
	if (RequestID == 0 || RequestID != PendingRequestID || ItemID != PendingItemId || ItemType != PendingItemType)
	{
		return;
	}
	if (auto* pc = Controller.Get())
	{
		pc->GetWorldTimerManager().ClearTimer(PurchaseResponseTimer);
	}
	PendingRequestID = 0;
	if (bReportPendingResult)
	{
		PurchaseOutcome = FShopPurchaseOutcome{bSuccess, ItemID, ItemType, FailureReason};
		UE_MVVM_SET_PROPERTY_VALUE(PurchaseResultText, bSuccess ? FString(TEXT("구매 완료")) : FailureReason);
	}
	UE_MVVM_SET_PROPERTY_VALUE(bPurchasePending, false);
	RefreshAvailability();
}

void UShopViewModel::OnPurchaseTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("Shop response timed out request=%u"), PendingRequestID);
	PendingRequestID = 0;
	if (bReportPendingResult)
	{
		UE_MVVM_SET_PROPERTY_VALUE(PurchaseResultText, FString(TEXT("구매 응답을 확인하지 못했습니다.")));
	}
	UE_MVVM_SET_PROPERTY_VALUE(bPurchasePending, false);
	RefreshAvailability();
}
