// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopComponent.h"

#include "CreditComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/GameManager/MatchGameMode.h"
#include "Valorant/GameManager/MatchGameState.h"
#include "Weapon/BaseWeapon.h"
#include "Kismet/GameplayStatics.h"

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

		// 상점 활성화 시 크레딧 동기화 요청
		if (bIsShopActive)
		{
			RequestCreditSync();
		}

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
	EWeaponCategory NewWeaponCategory = (*WeaponInfo)->WeaponCategory;

	// 플레이어 캐릭터 가져오기
	ABaseAgent* Agent = m_Owner->GetPawn<ABaseAgent>();
	if (!Agent) return false;

	// 무기 정보 확인
	ABaseWeapon* CurrentWeapon = nullptr;
	bool bHasSameIDWeapon = false;
	bool bHasSameCategoryWeapon = false;
	int32 RefundAmount = 0;

	// 새 무기가 Sidearm(보조무기)인 경우
	if (NewWeaponCategory == EWeaponCategory::Sidearm)
	{
		CurrentWeapon = Agent->GetSubWeapon();
		if (CurrentWeapon)
		{
			// 정확히 같은 무기인지 확인
			if (CurrentWeapon->GetWeaponID() == WeaponID)
			{
				bHasSameIDWeapon = true;
				// 같은 무기 ID는 이미 가지고 있으므로 구매 불가
				return false;
			}
			else
			{
				// 다른 무기지만 같은 카테고리(보조무기)인 경우
				bHasSameCategoryWeapon = true;

				// 무기 환불 금액 계산
				FWeaponData* CurrentWeaponData = GameInstance
					                                 ? GameInstance->GetWeaponData(CurrentWeapon->GetWeaponID())
					                                 : nullptr;

				if (CurrentWeaponData && !CurrentWeapon->GetWasUsed())
				{
					// 사용하지 않은 무기는 100% 환불
					RefundAmount = CurrentWeaponData->Cost;
				}
			}
		}
	}
	// 새 무기가 주무기인 경우(Sidearm이 아닌 모든 무기)
	else
	{
		CurrentWeapon = Agent->GetMainWeapon();
		if (CurrentWeapon)
		{
			// 정확히 같은 무기인지 확인
			if (CurrentWeapon->GetWeaponID() == WeaponID)
			{
				bHasSameIDWeapon = true;
				// 같은 무기 ID는 이미 가지고 있으므로 구매 불가
				return false;
			}
			else
			{
				// 다른 무기지만 같은 카테고리(주무기)인 경우
				bHasSameCategoryWeapon = true;

				// 무기 환불 금액 계산
				FWeaponData* CurrentWeaponData = GameInstance
					                                 ? GameInstance->GetWeaponData(CurrentWeapon->GetWeaponID())
					                                 : nullptr;

				if (CurrentWeaponData && !CurrentWeapon->GetWasUsed())
				{
					// 사용하지 않은 무기는 100% 환불
					RefundAmount = CurrentWeaponData->Cost;
				}
			}
		}
	}

	// 현재 보유 크레딧 + 환불 금액으로 구매 가능 여부 확인
	int32 CurrentCredits = CreditComp->GetCurrentCredit();
	bool bCanAffordAfterRefund = (CurrentCredits + RefundAmount) >= Cost;

	if (!bCanAffordAfterRefund)
	{
		// 환불액을 고려해도 구매 불가능한 경우
		return false;
	}

	// 여기서부터는 구매 가능한 케이스

	// 환불 금액을 크레딧에 추가
	if (bHasSameCategoryWeapon && RefundAmount > 0)
	{
		CreditComp->AddCredits(RefundAmount);
	}

	// 실제 지불해야 할 비용
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
				// OnShopItemPurchased.Broadcast(*Item); // 성공 여부를 서버에서 판단 후 Client_ReceivePurchaseResult를 통해 전달하므로 여기서는 제거하거나, 서버 전용 로깅/이벤트로 남길 수 있음
			}

			// 무기 생성 및 플레이어에게 할당
			SpawnWeaponForPlayer(WeaponID);

			// 클라이언트에 성공 결과 전송
			if (m_Owner) // m_Owner는 AAgentPlayerController
			{
				m_Owner->Client_ReceivePurchaseResult(true, WeaponID, EShopItemType::Weapon, TEXT(""));
			}
		}
		else
		{
			// 클라이언트에 실패 결과 전송 (크레딧 사용 실패)
			if (m_Owner)
			{
				m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
				                                      TEXT("Failed to use credits."));
			}
		}

		return bSuccess; // 이 반환 값은 서버 내부 로직용
	}
	// CanUseCredits(Cost) 실패 또는 이미 환불로도 구매 불가 처리됨
	else
	{
		// 클라이언트에 실패 결과 전송
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("Not enough credits even after refund."));
		}
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

				// OnShopItemPurchased.Broadcast(ArmorItem); // 성공 여부를 서버에서 판단 후 Client_ReceivePurchaseResult를 통해 전달
			}

			// 클라이언트에 성공 결과 전송
			if (m_Owner)
			{
				m_Owner->Client_ReceivePurchaseResult(true, ArmorLevel, EShopItemType::Armor, TEXT(""));
			}

			return bSuccess;
		}
	}
	else // 크레딧 사용 실패
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("Failed to use credits for armor."));
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

void UShopComponent::SpawnWeaponForPlayer(int32 WeaponID)
{
	if (!m_Owner)
	{
		return;
	}

	ABaseAgent* Agent = m_Owner->GetPawn<ABaseAgent>();
	if (!Agent)
	{
		return;
	}

	// 서버에서만 무기 생성 요청
	if (m_Owner->HasAuthority())
	{
		// 무기 데이터 가져오기
		FWeaponData** WeaponInfo = AvailableWeapons.Find(WeaponID);
		if (!WeaponInfo || !*WeaponInfo)
		{
			return;
		}

		EWeaponCategory WeaponCategory = (*WeaponInfo)->WeaponCategory;

		// 무기 카테고리에 따라 처리 방식 결정
		ABaseWeapon* CurrentWeapon = nullptr;
		if (WeaponCategory == EWeaponCategory::Sidearm)
		{
			CurrentWeapon = Agent->GetSubWeapon();
		}
		else
		{
			CurrentWeapon = Agent->GetMainWeapon();
		}

		// 이제 새 무기 생성 및 할당
		FVector SpawnLocation = Agent->GetActorLocation();
		FRotator SpawnRotation = Agent->GetActorRotation();

		// 무기 생성
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = Agent;

		ABaseWeapon* NewWeapon = GetWorld()->SpawnActor<ABaseWeapon>((*WeaponInfo)->WeaponClass, SpawnLocation,
		                                                             SpawnRotation, SpawnParams);

		if (NewWeapon)
		{
			// 무기 ID 설정
			//NewWeapon->SetWeaponID(WeaponID);
			// 무기 카테고리에 따라 장착 방식 결정

			if (CurrentWeapon)
			{
				// 기존 무기가 사용된 경우 드롭, 아닌 경우 제거
				if (CurrentWeapon->GetWasUsed())
				{
					CurrentWeapon->ServerRPC_Drop();
				}
				else
				{
					// 사용되지 않은 무기 제거
					// 환불은 이미 PurchaseWeapon에서 처리되었음
					switch (CurrentWeapon->GetWeaponCategory())
					{
					case EWeaponCategory::None:
						break;
					case EWeaponCategory::Melee:
						break;
					case EWeaponCategory::Sidearm:
						Agent->ResetSubWeapon();
					default:
						Agent->ResetMainWeapon();
					}
					
					CurrentWeapon->Destroy();
				}
			}

			Agent->ServerRPC_Interact(NewWeapon);
			// // 무기가 장착된 후 이벤트 발생
			// OnEquippedWeaponsChanged.Broadcast();
		}
	}
	else
	{
		// 클라이언트에서는 서버에 RPC 요청
		m_Owner->ServerRequestWeaponPurchase(WeaponID);
	}
	// 무기가 장착된 후 이벤트 발생
	OnEquippedWeaponsChanged.Broadcast();
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
