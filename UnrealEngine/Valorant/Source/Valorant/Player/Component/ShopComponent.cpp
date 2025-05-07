// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopComponent.h"

#include "CreditComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/GameManager/MatchGameMode.h"
#include "Valorant/GameManager/MatchGameState.h"

UShopComponent::UShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsShopOpen = false;
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

void UShopComponent::OpenShop()
{
	if (IsShopAvailable())
	{
		bIsShopOpen = true;
		// ToDo: 상점 UI 열기 로직
	}
}

void UShopComponent::CloseShop()
{
	bIsShopOpen = false;
	// ToDo: 상점 UI 닫기 로직
}

void UShopComponent::InitializeShopItems()
{
	if (!GameInstance)
	{
		return;
	}

	// 무기 목록 초기화
	TArray<FWeaponData*> WeaponList;
	GameInstance->GetAllWeaponData(WeaponList);

	for (FWeaponData* Weapon : WeaponList)
	{
		if (Weapon)
		{
			AvailableWeapons.Add(Weapon->WeaponID, Weapon);

			FShopItem NewItem;
			NewItem.ItemID = Weapon->WeaponID;
			//NewItem.ItemName = Weapon->WeaponName;
			NewItem.ItemType = EShopItemType::Weapon;
			NewItem.Price = Weapon->Cost;
			// ToDo: 아이콘 설정
			NewItem.bIsAvailable = true;

			ShopItems.Add(NewItem);
		}
	}

	// 능력 목록 초기화는 InitBySkillData에서 처리
}

void UShopComponent::InitBySkillData(TArray<int32> SkillIDs)
{
	if (!GameInstance)
	{
		return;
	}

	// 기존 능력 초기화
	AvailableAbilities.Empty();

	// 지정된 스킬 ID로 능력 데이터 로드
	for (int32 SkillID : SkillIDs)
	{
		FAbilityData* AbilityData = GameInstance->GetAbilityData(SkillID);
		if (AbilityData)
		{
			AvailableAbilities.Add(SkillID, AbilityData);

			FShopItem NewItem;
			NewItem.ItemID = AbilityData->AbilityID;
			NewItem.ItemName = AbilityData->AbilityName;
			NewItem.ItemType = EShopItemType::Ability;
			NewItem.Price = AbilityData->ChargeCost;
			NewItem.ItemIcon = AbilityData->AbilityIcon;
			NewItem.ItemClass = AbilityData->AbilityClass;
			NewItem.bIsAvailable = true;

			ShopItems.Add(NewItem);
		}
	}
}

bool UShopComponent::PurchaseWeapon(int32 WeaponID)
{
	// if (!IsShopAvailable() || !m_Owner)
	// {
	// 	return false;
	// }

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS) return false;

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp) return false;

	FWeaponData** WeaponInfo = AvailableWeapons.Find(WeaponID);
	if (!WeaponInfo || !*WeaponInfo)
	{
		return false;
	}

	int32 Cost = (*WeaponInfo)->Cost;

	// 크레딧 충분한지 확인
	if (CreditComp->CanUseCredits(Cost))
	{
		// 크레딧 차감 (서버에서 처리되어야 함)
		bool bSuccess = CreditComp->UseCredits(Cost);

		if (bSuccess)
		{
			// 구매 이벤트 발생
			FShopItem* Item = FindShopItem(WeaponID, EShopItemType::Weapon);
			if (Item)
			{
				OnShopItemPurchased.Broadcast(*Item);
			}
		}

		return bSuccess;
	}

	return false;
}

bool UShopComponent::PurchaseAbility(int32 AbilityID)
{
	if (!IsShopAvailable() || !m_Owner)
	{
		return false;
	}

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS) return false;

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp) return false;

	FAbilityData** AbilityInfo = AvailableAbilities.Find(AbilityID);
	if (!AbilityInfo || !*AbilityInfo)
	{
		return false;
	}

	int32 Cost = (*AbilityInfo)->ChargeCost;

	// 크레딧 충분한지 확인
	if (CreditComp->CanUseCredits(Cost))
	{
		// 서버에 구매 요청
		m_Owner->RequestPurchaseAbility(AbilityID);

		// 크레딧 차감은 실제 구매 성공 후 서버에서 처리됨
		// 여기서는 UI 업데이트를 위한 이벤트만 발생시킴
		FShopItem* Item = FindShopItem(AbilityID, EShopItemType::Ability);
		if (Item)
		{
			OnShopItemPurchased.Broadcast(*Item);
		}

		return true;
	}

	return false;
}

bool UShopComponent::PurchaseArmor(int32 ArmorLevel)
{
	if (!IsShopAvailable() || !m_Owner)
	{
		return false;
	}

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS) return false;

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp) return false;

	// 방어구 가격 설정 (레벨에 따라 다름)
	int32 Cost = 0;
	switch (ArmorLevel)
	{
	case 1: // 경장갑
		Cost = 400;
		break;
	case 2: // 중장갑
		Cost = 1000;
		break;
	// ToDo : 재생형 장갑
	default:
		return false;
	}

	// 크레딧 충분한지 확인
	if (CreditComp->CanUseCredits(Cost))
	{
		// 서버에 구매 요청
		if (m_Owner)
		{
			// ToDo: AAgentPlayerController에 방어구 구매 RPC 추가
			// m_Owner->RequestPurchaseArmor(ArmorLevel);

			// 크레딧 차감 (서버에서 처리되어야 함)
			bool bSuccess = CreditComp->UseCredits(Cost);

			if (bSuccess)
			{
				// 구매 이벤트 발생
				// 방어구는 동적으로 아이템 생성
				FShopItem ArmorItem;
				ArmorItem.ItemID = ArmorLevel;
				ArmorItem.ItemName = ArmorLevel == 1 ? "Light Armor" : "Heavy Armor";
				ArmorItem.ItemType = EShopItemType::Armor;
				ArmorItem.Price = Cost;

				OnShopItemPurchased.Broadcast(ArmorItem);
			}

			return bSuccess;
		}
	}

	return false;
}

bool UShopComponent::CanPurchaseItem(int32 ItemID, EShopItemType ItemType) const
{
	if (!m_Owner)
	{
		return false;
	}

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS) return false;

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp) return false;

	int32 Cost = 0;

	switch (ItemType)
	{
	case EShopItemType::Weapon:
		{
			const FWeaponData* const* WeaponInfo = AvailableWeapons.Find(ItemID);
			if (WeaponInfo && *WeaponInfo)
			{
				Cost = (*WeaponInfo)->Cost;
			}
		}
		break;

	case EShopItemType::Ability:
		{
			const FAbilityData* const* AbilityInfo = AvailableAbilities.Find(ItemID);
			if (AbilityInfo && *AbilityInfo)
			{
				Cost = (*AbilityInfo)->ChargeCost;
			}
		}
		break;

	case EShopItemType::Armor:
		Cost = (ItemID == 1) ? 400 : 1000;
		break;

	default:
		return false;
	}

	return CreditComp->CanUseCredits(Cost);
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
	if (!m_Owner)
	{
		return false;
	}

	// 게임 상태 확인
	AMatchGameState* GameState = m_Owner->GetWorld()->GetGameState<AMatchGameState>();
	if (!GameState)
	{
		return false;
	}

	// 현재 라운드 서브스테이트 확인 (구매 가능 단계인지)
	// ToDo: GameState에서 라운드 서브스테이트 확인 로직 추가
	// return GameState->GetRoundSubState() == ERoundSubState::RSS_BuyPhase;

	// 현재는 임시로 항상 true 반환
	return true;
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
