// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopComponent.h"

#include "CreditComponent.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/GameManager/MatchGameState.h"
#include "Weapon/BaseWeapon.h"

namespace
{
	bool IsShopWeapon(const FWeaponData& Weapon)
	{
		return Weapon.WeaponID > 0 && Weapon.WeaponClass && Weapon.WeaponMesh
			&& Weapon.WeaponCategory != EWeaponCategory::None && Weapon.WeaponCategory != EWeaponCategory::Melee;
	}

	float GetArmorCapacity(int32 ItemID)
	{
		return ItemID == 1 ? 25.f : 50.f;
	}

	int32 GetArmorCost(int32 ItemID)
	{
		switch (ItemID)
		{
		case 1: return 400;
		case 2: return 1000;
		default: return -1;
		}
	}
}

UShopComponent::UShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsShopActive = false;
}

void UShopComponent::BeginPlay()
{
	Super::BeginPlay();

	m_Owner = Cast<AAgentPlayerController>(GetOwner());

	if (m_Owner)
	{
		// 게임 인스턴스 가져오기
		GameInstance = Cast<UValorantGameInstance>(m_Owner->GetGameInstance());
	}

	// 상점 아이템 초기화
	InitializeShopItems();
}

void UShopComponent::SetShopActive(bool bActive)
{
	// 상점 이용 가능 상태 확인
	if (bActive && !IsShopAvailable())
	{
		// 이용 불가능한 상태에서 활성화 시도 시 무시
		return;
	}

	// 상태 변경이 있을 때만 처리
	if (bIsShopActive != bActive)
	{
		bIsShopActive = bActive;

		// 상태 변경 이벤트 발생
		OnShopAvailabilityChanged.Broadcast();
	}
}

void UShopComponent::RequestCreditSync()
{
	// 크레딧 동기화 요청 (서버에 최신 정보 요청)
	if (m_Owner)
	{
		AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
		if (PS)
		{
			// 서버에게 크레딧 동기화 요청
			PS->Server_RequestCreditSync();
		}
	}
}

void UShopComponent::InitializeShopItems()
{
	if (!GameInstance)
	{
		return;
	}

	ShopItems.Reset();
	AvailableWeapons.Reset();

	// 무기 목록 초기화
	TArray<FWeaponData*> WeaponList;
	GameInstance->GetAllWeaponData(WeaponList);

	for (FWeaponData* Weapon : WeaponList)
	{
		if (Weapon)
		{
			AvailableWeapons.Add(Weapon->WeaponID, Weapon);

			if (!IsShopWeapon(*Weapon))
			{
				continue;
			}

			FShopItem NewItem;
			NewItem.ItemID = Weapon->WeaponID;
			NewItem.ItemName = Weapon->LocalName.IsEmpty() ? UEnum::GetDisplayValueAsText(Weapon->WeaponName).ToString() : Weapon->LocalName;
			NewItem.ItemIcon = Weapon->WeaponIcon;
			NewItem.ItemClass = Weapon->WeaponClass;
			NewItem.ItemType = EShopItemType::Weapon;
			NewItem.Price = Weapon->Cost;
			NewItem.bIsAvailable = true;
			ShopItems.Add(NewItem);
		}
	}

	for (int32 itemId : {1, 2})
	{
		FShopItem& item = ShopItems.AddDefaulted_GetRef();
		item.ItemID = itemId;
		item.ItemName = itemId == 1 ? TEXT("경형 방어구") : TEXT("중형 방어구");
		item.ItemType = EShopItemType::Armor;
		item.Price = GetArmorCost(itemId);
	}
}

void UShopComponent::RefreshAbilityItems()
{
	ShopItems.RemoveAll([](const FShopItem& item) { return item.ItemType == EShopItemType::Ability; });

	const auto* state = m_Owner ? m_Owner->GetPlayerState<AAgentPlayerState>() : nullptr;
	const FAgentData* agent = GameInstance && state ? GameInstance->GetAgentData(state->GetAgentID()) : nullptr;
	if (!agent)
	{
		return;
	}

	for (const int32 abilityId : {agent->AbilityID_C, agent->AbilityID_Q, agent->AbilityID_E, agent->AbilityID_X})
	{
		if (abilityId != 0)
		{
			if (const FAbilityData* ability = GameInstance->GetAbilityData(abilityId); ability && ability->AbilityClass)
			{
				FShopItem& item = ShopItems.AddDefaulted_GetRef();
				item.ItemID = abilityId;
				item.ItemName = ability->AbilityName;
				item.ItemType = EShopItemType::Ability;
				item.Price = ability->ChargeCost;
				item.ItemIcon = ability->AbilityIcon;
				item.ItemClass = ability->AbilityClass;
			}
		}
	}
}

bool UShopComponent::IsAgentAbility(int32 AbilityID) const
{
	const auto* state = m_Owner ? m_Owner->GetPlayerState<AAgentPlayerState>() : nullptr;
	const FAgentData* agent = GameInstance && state ? GameInstance->GetAgentData(state->GetAgentID()) : nullptr;
	return AbilityID != 0 && agent
		&& (AbilityID == agent->AbilityID_C || AbilityID == agent->AbilityID_Q
			|| AbilityID == agent->AbilityID_E || AbilityID == agent->AbilityID_X);
}

bool UShopComponent::PurchaseWeapon(int32 WeaponID)
{
	return PurchaseItem(WeaponID, EShopItemType::Weapon, 0);
}

bool UShopComponent::PurchaseAbility(int32 AbilityID)
{
	return PurchaseItem(AbilityID, EShopItemType::Ability, 0);
}

bool UShopComponent::PurchaseArmor(int32 ArmorLevel)
{
	return PurchaseItem(ArmorLevel, EShopItemType::Armor, 0);
}

bool UShopComponent::PurchaseItem(int32 ItemID, EShopItemType ItemType, uint32 RequestID)
{
	if (!m_Owner || !m_Owner->HasAuthority())
	{
		return false;
	}
	if (RequestID != 0)
	{
		if (RequestID <= LatestRequestID)
		{
			SendPurchaseResponse(RequestID, ItemID, ItemType, TEXT("이미 처리한 요청 번호입니다."));
			return false;
		}
		LatestRequestID = RequestID;
	}
	int32 refund = 0;
	const TCHAR* failure = GetPurchaseError(ItemID, ItemType, refund);
	if (!failure)
	{
		auto* state = m_Owner->GetPlayerState<AAgentPlayerState>();
		auto* credits = state->GetCreditComponent();
		switch (ItemType)
		{
		case EShopItemType::Weapon:
			{
				const FWeaponData& weapon = *AvailableWeapons.FindChecked(ItemID);
				if (SpawnWeaponForPlayer(weapon))
				{
					credits->AddCredits(refund - weapon.Cost);
					OnEquippedWeaponsChanged.Broadcast();
				}
				else
				{
					failure = TEXT("무기를 장착할 수 없습니다.");
				}
			}
			break;
		case EShopItemType::Ability:
			if (credits->UseCredits(GameInstance->GetAbilityData(ItemID)->ChargeCost))
			{
				state->AddAbilityStack(ItemID);
			}
			else
			{
				failure = TEXT("크레딧 사용에 실패했습니다.");
			}
			break;
		case EShopItemType::Armor:
			if (credits->UseCredits(GetArmorCost(ItemID)))
			{
				state->SetShopArmorTier(static_cast<uint8>(ItemID));
				state->GetBaseAttributeSet()->SetArmor(GetArmorCapacity(ItemID));
			}
			else
			{
				failure = TEXT("크레딧 사용에 실패했습니다.");
			}
			break;
		default:
			checkNoEntry();
			break;
		}
	}

	if (!failure)
	{
		if (const FShopItem* item = FindShopItem(ItemID, ItemType))
		{
			OnShopItemPurchased.Broadcast(*item);
		}
	}
	SendPurchaseResponse(RequestID, ItemID, ItemType, failure);
	return !failure;
}

void UShopComponent::SendPurchaseResponse(uint32 RequestID, int32 ItemID, EShopItemType ItemType, const TCHAR* Failure) const
{
	const TCHAR* reason = Failure ? Failure : TEXT("");
	if (RequestID != 0)
	{
		m_Owner->Client_ReceiveShopPurchase(RequestID, !Failure, ItemID, ItemType, reason);
	}
	else
	{
		m_Owner->Client_ReceivePurchaseResult(!Failure, ItemID, ItemType, reason);
	}
}

bool UShopComponent::CanPurchaseItem(int32 ItemID, EShopItemType ItemType, int32& OutRefundAmount) const
{
	return GetPurchaseError(ItemID, ItemType, OutRefundAmount) == nullptr;
}

const TCHAR* UShopComponent::GetPurchaseError(int32 ItemID, EShopItemType ItemType, int32& OutRefundAmount) const
{
	OutRefundAmount = 0;
	if (!IsShopAvailable())
	{
		return TEXT("현재 상점을 이용할 수 없습니다.");
	}
	const auto* state = m_Owner->GetPlayerState<AAgentPlayerState>();
	const auto* credits = state ? state->GetCreditComponent() : nullptr;
	if (!credits)
	{
		return TEXT("크레딧 정보를 찾을 수 없습니다.");
	}

	int32 cost = -1;
	switch (ItemType)
	{
	case EShopItemType::Weapon:
		{
			const auto* const* row = AvailableWeapons.Find(ItemID);
			if (!row || !*row || !IsShopWeapon(**row))
			{
				return TEXT("유효하지 않은 무기입니다.");
			}
			const auto& weapon = **row;
			cost = weapon.Cost;
			const auto* agent = m_Owner->GetPawn<ABaseAgent>();
			const auto* current = weapon.WeaponCategory == EWeaponCategory::Sidearm ? agent->GetSubWeapon() : agent->GetMainWeapon();
			if (IsValid(current))
			{
				if (current->GetWeaponID() == ItemID)
				{
					return TEXT("이미 보유 중인 무기입니다.");
				}
				if (!current->GetWasUsed())
				{
					if (const auto* data = GameInstance->GetWeaponData(current->GetWeaponID()))
					{
						OutRefundAmount = FMath::Max(0, data->Cost);
					}
				}
			}
		}
		break;
	case EShopItemType::Ability:
		{
			const auto* ability = IsAgentAbility(ItemID) ? GameInstance->GetAbilityData(ItemID) : nullptr;
			if (!ability || !ability->AbilityClass)
			{
				return TEXT("유효하지 않은 스킬입니다.");
			}
			if (state->GetAbilityStack(ItemID) >= state->GetMaxAbilityStack(ItemID))
			{
				return TEXT("이미 최대 개수의 스킬을 보유하고 있습니다.");
			}
			cost = ability->ChargeCost;
		}
		break;
	case EShopItemType::Armor:
		{
			cost = GetArmorCost(ItemID);
			const auto* attributes = state->GetBaseAttributeSet();
			if (!attributes || cost < 0)
			{
				return TEXT("유효하지 않은 방어구입니다.");
			}
			if (attributes->GetArmor() >= GetArmorCapacity(ItemID))
			{
				return TEXT("현재 방어구 수치가 구매하려는 방어구 이상입니다.");
			}
			if (attributes->GetArmor() > 0.f && ItemID < state->GetShopArmorTier())
			{
				return TEXT("보유한 방어구 등급보다 낮은 등급은 구매할 수 없습니다.");
			}
		}
		break;
	default:
		return TEXT("유효하지 않은 상품입니다.");
	}

	return cost >= 0 && credits->CanUseCredits(FMath::Max(0, cost - OutRefundAmount))
		? nullptr : TEXT("크레딧이 부족합니다.");
}

FShopItem* UShopComponent::FindShopItem(int32 ItemID, EShopItemType ItemType)
{
	for (int32 i = 0; i < ShopItems.Num(); i++)
	{
		if (ShopItems[i].ItemID == ItemID && ShopItems[i].ItemType == ItemType)
		{
			return &ShopItems[i];
		}
	}
	return nullptr;
}

bool UShopComponent::IsShopAvailable() const
{
	const auto* agent = m_Owner ? m_Owner->GetPawn<ABaseAgent>() : nullptr;
	const auto* match = GetWorld() ? GetWorld()->GetGameState<AMatchGameState>() : nullptr;
	return agent && !agent->IsDead() && match && match->CanOpenShop();
}

TArray<FShopItem> UShopComponent::GetShopItemsByType(EShopItemType ItemType) const
{
	TArray<FShopItem> FilteredItems;

	for (const FShopItem& Item : ShopItems)
	{
		if (Item.ItemType == ItemType)
		{
			FilteredItems.Add(Item);
		}
	}

	return FilteredItems;
}

bool UShopComponent::SpawnWeaponForPlayer(const FWeaponData& WeaponData)
{
	auto* agent = m_Owner->GetPawn<ABaseAgent>();
	const FTransform transform(agent->GetActorRotation(), agent->GetActorLocation() + FVector(0, 0, 1000));
	auto* weapon = GetWorld()->SpawnActorDeferred<ABaseWeapon>(WeaponData.WeaponClass, transform);
	if (!IsValid(weapon))
	{
		return false;
	}
	weapon->SetWeaponID(WeaponData.WeaponID);
	weapon->FinishSpawning(transform);
	if (!IsValid(weapon))
	{
		return false;
	}
	const auto slot = WeaponData.WeaponCategory == EWeaponCategory::Sidearm ? EInteractorType::SubWeapon : EInteractorType::MainWeapon;
	if (weapon->GetInteractorType() == slot && weapon->TryEquipPurchasedWeapon(*agent))
	{
		return true;
	}
	weapon->Destroy();
	return false;
}

TArray<int32> UShopComponent::GetEquippedWeaponIDs() const
{
	TArray<int32> EquippedWeaponIDs;

	if (!m_Owner)
	{
		return EquippedWeaponIDs;
	}

	ABaseAgent* Agent = m_Owner->GetPawn<ABaseAgent>();
	if (!Agent)
	{
		return EquippedWeaponIDs;
	}

	// 주무기 확인
	ABaseWeapon* MainWeapon = Agent->GetMainWeapon();
	if (MainWeapon)
	{
		EquippedWeaponIDs.Add(MainWeapon->GetWeaponID());
	}

	// 보조무기 확인
	ABaseWeapon* SubWeapon = Agent->GetSubWeapon();
	if (SubWeapon)
	{
		EquippedWeaponIDs.Add(SubWeapon->GetWeaponID());
	}

	return EquippedWeaponIDs;
}

bool UShopComponent::IsWeaponEquipped(int32 WeaponID) const
{
	if (!m_Owner)
	{
		return false;
	}

	ABaseAgent* Agent = m_Owner->GetPawn<ABaseAgent>();
	if (!Agent)
	{
		return false;
	}

	// 주무기 확인
	ABaseWeapon* PrimaryWeapon = Agent->GetMainWeapon();
	if (PrimaryWeapon && PrimaryWeapon->GetWeaponID() == WeaponID)
	{
		return true;
	}

	// 보조무기 확인
	ABaseWeapon* SecondWeapon = Agent->GetSubWeapon();
	if (SecondWeapon && SecondWeapon->GetWeaponID() == WeaponID)
	{
		return true;
	}

	return false;
}

// 어빌리티 스택 정보 가져오기 구현
int32 UShopComponent::GetAbilityStack(int32 AbilityID) const
{
	if (!m_Owner)
	{
		return 0;
	}

	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return 0;
	}

	return PS->GetAbilityStack(AbilityID);
}

// 어빌리티의 최대 스택 수 가져오기 구현
int32 UShopComponent::GetMaxAbilityStack(int32 AbilityID) const
{
	if (!m_Owner)
	{
		return 0;
	}

	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return 0;
	}

	return PS->GetMaxAbilityStack(AbilityID);
}
