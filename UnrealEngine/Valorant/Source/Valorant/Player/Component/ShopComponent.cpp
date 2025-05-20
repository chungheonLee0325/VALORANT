// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopComponent.h"

#include "CreditComponent.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
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
			NewItem.ItemType = EShopItemType::Weapon;
			NewItem.Price = Weapon->Cost;
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

	// 지정된 스킬 ID로 능력 데이터 로드
	for (int32 SkillID : SkillIDs)
	{
		FAbilityData* AbilityData = GameInstance->GetAbilityData(SkillID);
		if (AbilityData)
		{
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
	if (!IsShopAvailable() || !m_Owner)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("상점이 현재 이용할 수 없습니다."));
		}
		return false;
	}

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("플레이어 정보를 찾을 수 없습니다."));
		}
		return false;
	}

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("크레딧 정보를 찾을 수 없습니다."));
		}
		return false;
	}

	FWeaponData** WeaponInfo = AvailableWeapons.Find(WeaponID);
	if (!WeaponInfo || !*WeaponInfo)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("유효하지 않은 무기입니다."));
		}
		return false;
	}

	//구매 가능 여부 및 환불 금액 확인
	int32 RefundAmount = 0;
	if (!CanPurchaseItem(WeaponID, EShopItemType::Weapon, RefundAmount))
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("구매할 수 없는 무기입니다. 크레딧이 부족하거나 이미 보유 중입니다."));
		}
		return false;
	}

	int32 Cost = (*WeaponInfo)->Cost;

	// 여기서부터는 구매 가능한 케이스
	// 환불 금액이 있으면 크레딧에 추가
	if (RefundAmount > 0)
	{
		CreditComp->AddCredits(RefundAmount);
	}

	// 비용 지불
	if (CreditComp->CanUseCredits(Cost))
	{
		// 크레딧 차감
		bool bSuccess = CreditComp->UseCredits(Cost);

		if (bSuccess)
		{
			// 구매 이벤트 발생
			FShopItem* Item = FindShopItem(WeaponID, EShopItemType::Weapon);
			if (Item)
			{
				OnShopItemPurchased.Broadcast(*Item);
			}

			// 무기 생성 및 플레이어에게 할당
			SpawnWeaponForPlayer(WeaponID);

			// 클라이언트에 성공 결과 전송
			if (m_Owner)
			{
				m_Owner->Client_ReceivePurchaseResult(true, WeaponID, EShopItemType::Weapon, TEXT(""));
			}

			return true;
		}
		else
		{
			// 클라이언트에 실패 결과 전송 (크레딧 사용 실패)
			if (m_Owner)
			{
				m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
				                                      TEXT("크레딧 사용에 실패했습니다."));
			}
		}
	}
	else
	{
		// 클라이언트에 실패 결과 전송
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, WeaponID, EShopItemType::Weapon,
			                                      TEXT("환불액을 고려해도 크레딧이 부족합니다."));
		}
	}

	return false;
}

bool UShopComponent::PurchaseAbility(int32 AbilityID)
{
	if (!IsShopAvailable() || !m_Owner)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("상점이 현재 이용할 수 없습니다."));
		}
		return false;
	}

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("플레이어 정보를 찾을 수 없습니다."));
		}
		return false;
	}

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("크레딧 정보를 찾을 수 없습니다."));
		}
		return false;
	}

	FAbilityData* AbilityInfo = GameInstance->GetAbilityData(AbilityID);
	if (!AbilityInfo)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("유효하지 않은 스킬입니다."));
		}
		return false;
	}

	// 현재 스택이 최대치인지 확인
	if (PS->GetAbilityStack(AbilityID) >= PS->GetMaxAbilityStack(AbilityID))
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("이미 최대 개수의 스킬을 보유하고 있습니다."));
		}
		return false;
	}

	// 개선된 CanPurchaseItem 사용
	int32 RefundAmount = 0; // 어빌리티는 환불 없음
	if (!CanPurchaseItem(AbilityID, EShopItemType::Ability, RefundAmount))
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("크레딧이 부족합니다."));
		}
		return false;
	}

	int32 Cost = AbilityInfo->ChargeCost;

	// 크레딧 사용
	bool bSuccess = CreditComp->UseCredits(Cost);
	if (bSuccess)
	{
		// 스택 증가 - 사용자 변경 유지
		PS->AddAbilityStack(AbilityID);

		// 구매 성공 이벤트 발생
		FShopItem* Item = FindShopItem(AbilityID, EShopItemType::Ability);
		if (Item)
		{
			OnShopItemPurchased.Broadcast(*Item);
		}

		// 클라이언트에 성공 결과 전송
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(true, AbilityID, EShopItemType::Ability, TEXT(""));
		}

		return true;
	}
	else
	{
		// 크레딧 사용 실패
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, AbilityID, EShopItemType::Ability,
			                                      TEXT("크레딧 사용에 실패했습니다."));
		}
		return false;
	}
}

bool UShopComponent::PurchaseArmor(int32 ArmorLevel)
{
	if (!IsShopAvailable() || !m_Owner)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("상점이 현재 이용할 수 없습니다."));
		}
		return false;
	}

	// 플레이어 스테이트와 크레딧 컴포넌트 찾기
	AAgentPlayerState* PS = m_Owner->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("플레이어 정보를 찾을 수 없습니다."));
		}
		return false;
	}

	UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
	if (!CreditComp)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("크레딧 정보를 찾을 수 없습니다."));
		}
		return false;
	}

	// 방어구 레벨 확인
	if (ArmorLevel != 1 && ArmorLevel != 2)
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("유효하지 않은 방어구 레벨입니다."));
		}
		return false;
	}

	// 개선된 CanPurchaseItem 사용
	int32 RefundAmount = 0; // 방어구는 환불 없음
	if (!CanPurchaseItem(ArmorLevel, EShopItemType::Armor, RefundAmount))
	{
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("크레딧이 부족합니다."));
		}
		return false;
	}

	int32 Cost = (ArmorLevel == 1) ? 400 : 1000;

	// 크레딧 사용
	bool bSuccess = CreditComp->UseCredits(Cost);
	if (bSuccess)
	{
		// 방어구 적용 로직
		UBaseAttributeSet* AttributeSet = PS->GetBaseAttributeSet();
		if (AttributeSet)
		{
			float ArmorValue = (ArmorLevel == 1) ? 25.0f : 50.0f;
			AttributeSet->SetArmor(ArmorValue);
		}

		// 구매 성공 이벤트 발생
		FShopItem ArmorItem;
		ArmorItem.ItemID = ArmorLevel;
		ArmorItem.ItemName = ArmorLevel == 1 ? "Light Armor" : "Heavy Armor";
		ArmorItem.ItemType = EShopItemType::Armor;
		ArmorItem.Price = Cost;
		OnShopItemPurchased.Broadcast(ArmorItem);

		// 클라이언트에 성공 결과 전송
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(true, ArmorLevel, EShopItemType::Armor, TEXT(""));
		}

		return true;
	}
	else
	{
		// 크레딧 사용 실패
		if (m_Owner)
		{
			m_Owner->Client_ReceivePurchaseResult(false, ArmorLevel, EShopItemType::Armor,
			                                      TEXT("크레딧 사용에 실패했습니다."));
		}
		return false;
	}
}

bool UShopComponent::CanPurchaseItem(int32 ItemID, EShopItemType ItemType, int32& OutRefundAmount) const
{
	OutRefundAmount = 0; // 기본값으로 초기화

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
	bool bCanPurchase = true;

	switch (ItemType)
	{
	case EShopItemType::Weapon:
		{
			const FWeaponData* const* WeaponInfo = AvailableWeapons.Find(ItemID);
			if (WeaponInfo && *WeaponInfo)
			{
				Cost = (*WeaponInfo)->Cost;

				// 무기에만 환불 로직 적용
				ABaseAgent* Agent = m_Owner->GetPawn<ABaseAgent>();
				if (Agent)
				{
					EWeaponCategory NewWeaponCategory = (*WeaponInfo)->WeaponCategory;
					ABaseWeapon* CurrentWeapon = nullptr;

					// 무기 카테고리에 따라 처리
					if (NewWeaponCategory == EWeaponCategory::Melee)
					{
						if (Agent->GetMeleeWeapon() != nullptr) return false;
					}
					else if (NewWeaponCategory == EWeaponCategory::Sidearm)
					{
						CurrentWeapon = Agent->GetSubWeapon();
					}
					else
					{
						CurrentWeapon = Agent->GetMainWeapon();
					}

					// 같은 무기를 이미 소유한 경우 구매 불가
					if (CurrentWeapon && CurrentWeapon->GetWeaponID() == ItemID)
					{
						return false;
					}

					// 같은 카테고리의 다른 무기 환불 계산
					if (CurrentWeapon && CurrentWeapon->GetWeaponID() != ItemID)
					{
						FWeaponData* CurrentWeaponData = GameInstance
							                                 ? GameInstance->GetWeaponData(CurrentWeapon->GetWeaponID())
							                                 : nullptr;

						if (CurrentWeaponData && !CurrentWeapon->GetWasUsed())
						{
							OutRefundAmount = CurrentWeaponData->Cost;
						}
					}
				}
			}
			else
			{
				bCanPurchase = false;
			}
		}
		break;

	case EShopItemType::Ability:
		{
			const FAbilityData* const AbilityInfo = GameInstance->GetAbilityData(ItemID);
			if (AbilityInfo)
			{
				Cost = AbilityInfo->ChargeCost;

				// 어빌리티는 현재 스택과 최대 스택 확인 - 사용자 변경 로직 유지
				int32 CurrentStack = PS->GetAbilityStack(ItemID);
				int32 MaxStack = PS->GetMaxAbilityStack(ItemID);

				// 스택이 이미 최대이면 구매 불가
				if (CurrentStack >= MaxStack)
				{
					return false;
				}
			}
			else
			{
				bCanPurchase = false;
			}
		}
		break;

	case EShopItemType::Armor:
		{
			// 방어구는 간단한 가격 확인만 수행
			if (ItemID == 1) // 경장갑
			{
				Cost = 400;
			}
			else if (ItemID == 2) // 중장갑
			{
				Cost = 1000;
			}
			else
			{
				bCanPurchase = false;
			}

			// 미래 확장: 현재 방어구 상태 확인 로직 추가 가능
		}
		break;

	default:
		bCanPurchase = false;
		break;
	}

	// 구매 불가 조건
	if (!bCanPurchase)
	{
		return false;
	}

	// 할인된 가격으로 구매 가능 여부 확인
	int32 FinalCost = Cost - OutRefundAmount;
	return CreditComp->CanUseCredits(FinalCost);
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
	return GameState->GetRoundSubState() == ERoundSubState::RSS_BuyPhase || GameState->GetRoundSubState() ==
		ERoundSubState::RSS_PreRound;
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
		if (WeaponCategory == EWeaponCategory::Melee)
		{
			if (Agent->GetMeleeWeapon() != nullptr) return;;
		}
		else if (WeaponCategory == EWeaponCategory::Sidearm)
		{
			CurrentWeapon = Agent->GetSubWeapon();
		}
		else
		{
			CurrentWeapon = Agent->GetMainWeapon();
		}

		// 이제 새 무기 생성 및 할당
		FVector SpawnLocation = Agent->GetActorLocation() - FVector(0, 0, -1000);
		FRotator SpawnRotation = Agent->GetActorRotation();

		// 무기 생성
		// FActorSpawnParameters SpawnParams;
		// SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// SpawnParams.Owner = Agent;

		const FTransform NewTransform = FTransform(SpawnRotation, SpawnLocation);
		ABaseWeapon* NewWeapon = GetWorld()->SpawnActorDeferred<ABaseWeapon>((*WeaponInfo)->WeaponClass, NewTransform);
		if (IsValid(NewWeapon))
		{
			// KBD: BeginPlay 실행을 지연시키고 ID 설정
			NewWeapon->SetWeaponID(WeaponID);
			// BeginPlay 실행
			NewWeapon->FinishSpawning(NewTransform);
		}
		
		if (NewWeapon)
		{
			// NewWeapon->NetMulti_ReloadWeaponData(WeaponID);
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
