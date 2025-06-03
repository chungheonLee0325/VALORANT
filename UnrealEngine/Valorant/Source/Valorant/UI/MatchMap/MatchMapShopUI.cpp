// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapShopUI.h"
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/Component/ShopComponent.h"
#include "Valorant/Player/Component/CreditComponent.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Valorant/GameManager/ValorantGameInstance.h"

void UMatchMapShopUI::NativeConstruct()
{
	Super::NativeConstruct();

	// UI 초기화 설정
	if (CreditText)
	{
		// 기본값으로 0 설정
		AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
		if (PS)
		{
			int32 currentCredit = PS->GetCurrentCredit();
			UpdateCreditDisplay(currentCredit);
		}
		OriginalCreditTextColor = CreditText->GetColorAndOpacity();
	}

	GameInstance = UValorantGameInstance::Get(GetWorld());

	// 색상 관련 초기화
	if (CreditText)
	{
		OriginalCreditTextColor = CreditText->GetColorAndOpacity();
	}
}

void UMatchMapShopUI::InitializeShopUI(AAgentPlayerController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	OwnerController = Controller;

	// 초기 크레딧 표시 업데이트
	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		// PlayerState의 크레딧 델리게이트에 바인딩
		PS->OnCreditChangedDelegate.AddDynamic(this, &UMatchMapShopUI::UpdateCreditDisplay);

		// 어빌리티 스택 변경 델리게이트에 바인딩
		PS->OnAbilityStackChanged.AddDynamic(this, &UMatchMapShopUI::HandleAbilityStackChanged);

		// 크레딧 컴포넌트의 델리게이트에도 바인딩 (이중 안전장치)
		UCreditComponent* CreditComp = PS->GetCreditComponent();
		if (CreditComp)
		{
			// 서버에 최신 크레딧 값 요청 (명시적 동기화)
			RequestLatestCreditValue();

			// 크레딧 변경 이벤트에 바인딩
			CreditComp->OnCreditChanged.AddDynamic(this, &UMatchMapShopUI::UpdateCreditDisplay);
		}

		// 서버에 스택 정보 요청
		PS->Server_RequestAbilityStackSync();
	}

	// 상점 아이템 목록 업데이트
	UpdateShopItemList();

	// 상점 컴포넌트의 무기 장착 변경 이벤트에 연결
	if (Controller && Controller->ShopComponent)
	{
		Controller->ShopComponent->OnEquippedWeaponsChanged.AddDynamic(this, &UMatchMapShopUI::Client_UpdateWeaponHighlight);

		// 초기 하이라이트 상태 업데이트
		Client_UpdateWeaponHighlight();
	}

	// 서버로부터 오는 구매 결과 델리게이트에 바인딩
	if (OwnerController)
	{
		OwnerController->OnServerPurchaseResult.AddDynamic(this, &UMatchMapShopUI::HandleServerPurchaseResult);
	}

	// 어빌리티 UI 초기화
	InitializeAbilityUI();
	
	// 어빌리티 데이터 로드 및 UI 업데이트
	LoadAllAbilityData();
	UpdateAbilityUI();
}

void UMatchMapShopUI::UpdateCreditDisplay(int32 CurrentCredit)
{
	// 현재 크레딧 저장
	CurrentCredits = CurrentCredit;

	// 크레딧 텍스트 업데이트
	if (CreditText)
	{
		// 크레딧 금액 포맷팅 (예: 1,000)
		FString FormattedCredit = FString::Printf(TEXT("%d"), CurrentCredit);

		// 1000 단위로 콤마 추가
		for (int32 i = FormattedCredit.Len() - 3; i > 0; i -= 3)
		{
			FormattedCredit.InsertAt(i, TEXT(","));
		}

		CreditText->SetText(FText::FromString(FormattedCredit));
	}
}

void UMatchMapShopUI::OnClickedBuyWeaponButton(const int WeaponId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	// 구매 전 크레딧 충분한지 확인
	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
		UShopComponent* ShopComp = OwnerController->ShopComponent;

		if (CreditComp && ShopComp)
		{
			// 아이템 가격 확인
			const FShopItem* Item = nullptr;
			for (const FShopItem& ShopItem : ShopComp->GetAllShopItems())
			{
				if (ShopItem.ItemID == WeaponId && ShopItem.ItemType == EShopItemType::Weapon)
				{
					Item = &ShopItem;
					break;
				}
			}

			// 크레딧 부족 시 UI 피드백
			int refundAmount = 0;
			if (!ShopComp->CanPurchaseItem(Item->ItemID, Item->ItemType, refundAmount))
			{
				// 색깔 피드백
				FlashCreditTextColor(FLinearColor::Red);

				// 구매 실패 처리
				HandleServerPurchaseResult(false, WeaponId, EShopItemType::Weapon, TEXT("크레딧이 부족합니다."));
				return;
			}
		}
	}

	// 구매 요청
	OwnerController->RequestPurchaseWeapon(WeaponId);
}

void UMatchMapShopUI::OnClickedBuySkillButton(const int SkillId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	// 현재 스택 및 최대 스택 확인
	int32 CurrentStack = GetAbilityStack(SkillId);
	int32 MaxStack = GetMaxAbilityStack(SkillId);

	// 이미 최대 스택인 경우 구매 불가 메시지 표시
	if (CurrentStack >= MaxStack)
	{
		// 구매 실패 피드백 - 이미 최대 스택
		HandleServerPurchaseResult(false, SkillId, EShopItemType::Ability, TEXT("최대 스택에 도달했습니다."));
		return;
	}

	// 구매 전 크레딧 충분한지 확인
	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
		UShopComponent* ShopComp = OwnerController->ShopComponent;

		if (CreditComp && ShopComp)
		{
			// 아이템 가격 확인
			FAbilityData* AbilityData = GameInstance->GetAbilityData(SkillId);
			if (AbilityData && !CreditComp->CanUseCredits(AbilityData->ChargeCost))
			{
				// 색깔 피드백
				FlashCreditTextColor(FLinearColor::Red);

				// 구매 실패 처리
				HandleServerPurchaseResult(false, SkillId, EShopItemType::Ability, TEXT("크레딧이 부족합니다."));
				return;
			}
		}
	}

	// 컨트롤러를 통해 스킬 구매 요청 (ShopComponent를 직접 호출하지 않음)
	OwnerController->RequestPurchaseAbility(SkillId);
}

void UMatchMapShopUI::OnClickedBuyShiledButton(const int ShieldId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	// 구매 전 크레딧 충분한지 확인
	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
		if (CreditComp)
		{
			// 아이템 가격 확인 (방어구는 ID 1이면 400, 2면 1000)
			int32 Cost = (ShieldId == 1) ? 400 : 1000;

			if (!CreditComp->CanUseCredits(Cost))
			{
				// 색깔 피드백
				FlashCreditTextColor(FLinearColor::Red);

				// 구매 실패 처리
				HandleServerPurchaseResult(false, ShieldId, EShopItemType::Armor, TEXT("크레딧이 부족합니다."));
				return;
			}
		}
	}

	// 컨트롤러를 통해 방어구 구매 요청
	OwnerController->RequestPurchaseArmor(ShieldId);
}

void UMatchMapShopUI::Client_UpdateWeaponHighlight_Implementation()
{
	UpdateWeaponHighlights();
}

void UMatchMapShopUI::UpdateShopItemList()
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	// ShopComponent에서 모든 아이템 목록 가져오기
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		// Blueprint에서 구현할 아이템 목록 업데이트 로직
		// 블루프린트 이벤트를 호출하는 방식으로 구현하는 것이 좋음
	}
}

void UMatchMapShopUI::UpdateShopItemListByType(EShopItemType ItemType)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	// ShopComponent에서 특정 타입 아이템 목록 가져오기
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		// 특정 타입의 아이템만 필터링
		TArray<FShopItem> FilteredItems = ShopComp->GetShopItemsByType(ItemType);

		// Blueprint에서 구현할 아이템 목록 업데이트 로직
		// 블루프린트 이벤트를 호출하는 방식으로 구현하는 것이 좋음
	}
}

void UMatchMapShopUI::HandleServerPurchaseResult(bool bSuccess, int32 ItemID, EShopItemType ItemType,
                                                 const FString& FailureReason)
{
	UE_LOG(LogTemp, Log, TEXT("HandleServerPurchaseResult: Success: %d, ItemID: %d, Type: %s, Reason: %s"), bSuccess,
	       ItemID, *UEnum::GetValueAsString(ItemType), *FailureReason);

	// 구매 결과 이벤트 발생 (Blueprint 등에서 사용 가능)
	OnPurchaseResult.Broadcast(bSuccess, ItemID, ItemType);

	if (bSuccess)
	{
		Client_UpdateWeaponHighlight();
		UpdateShopItemList(); // 아이템 목록 UI 갱신
		// 크레딧 표시는 OnCreditChanged 델리게이트를 통해 자동으로 업데이트될 것이므로, 여기서 직접 호출할 필요는 없을 수 있음
		// 필요하다면 RequestLatestCreditValue(); 호출 고려
		UE_LOG(LogTemp, Log, TEXT("Purchase successful for Item %d (%s)"), ItemID, *UEnum::GetValueAsString(ItemType));
		// ToDo : 성공 시 UI 피드백 로직 추가 (예: 성공 메시지, 사운드)
		
		// 어빌리티 구매 성공 시 해당 어빌리티 정보 업데이트
		if (ItemType == EShopItemType::Ability)
		{
			LoadAbilityData(ItemID);
			UpdateAbilityUI();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Purchase failed for Item %d (%s). Reason: %s"), ItemID,
		       *UEnum::GetValueAsString(ItemType), *FailureReason);
		// 여기에 실패 시 UI 피드백 로직 추가 (예: 실패 메시지 표시 - FailureReason 활용)
		// 예를 들어, 특정 TextBlock에 FailureReason을 표시할 수 있습니다.
	}
}

// 서버에 최신 크레딧 정보 요청 (명시적 동기화)
void UMatchMapShopUI::RequestLatestCreditValue()
{
	if (!OwnerController)
	{
		return;
	}

	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}

	// 서버에서 최신 크레딧 값을 강제로 불러오는 RPC 호출
	// 이 함수는 AAgentPlayerState에 추가되어야 함
	PS->Server_RequestCreditSync();
}

void UMatchMapShopUI::UpdateWeaponHighlights()
{
	if (!OwnerController || !OwnerController->ShopComponent)
	{
		return;
	}

	// Replicate 동기화를 위해 잠시뒤 처리
	FTimerHandle TimerHandle;
	TWeakObjectPtr<UMatchMapShopUI> WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakThis]()
	{
		UMatchMapShopUI* StrongThis = WeakThis.Get();
		if (StrongThis)
		{
			// 현재 장착 중인 무기 ID 가져오기
			StrongThis->EquippedWeaponIDs = StrongThis->OwnerController->ShopComponent->GetEquippedWeaponIDs();

			// Blueprint에서 구현할 UI 업데이트 이벤트 호출
			StrongThis->OnWeaponHighlightUpdated();
		}
	}
	, 0.1f, false);
}

bool UMatchMapShopUI::IsWeaponEquipped(int32 WeaponID) const
{
	return EquippedWeaponIDs.Contains(WeaponID);
}

// 어빌리티 스택 관련 함수 구현
int32 UMatchMapShopUI::GetAbilityStack(int32 AbilityID) const
{
	if (!OwnerController)
	{
		return 0;
	}

	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return 0;
	}

	return PS->GetAbilityStack(AbilityID);
}

int32 UMatchMapShopUI::GetMaxAbilityStack(int32 AbilityID) const
{
	if (!OwnerController)
	{
		return 0;
	}

	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return 0;
	}

	return PS->GetMaxAbilityStack(AbilityID);
}

void UMatchMapShopUI::UpdateAbilityStacks()
{
	if (!OwnerController)
	{
		return;
	}

	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}

	// 현재 캐릭터의 모든 어빌리티 ID 가져오기
	TArray<int32> AbilityIDs;
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		// 스킬 슬롯별 어빌리티 ID 가져오기
		int32 AgentID = PS->GetAgentID();
		if (GameInstance)
		{
			FAgentData* AgentData = GameInstance->GetAgentData(AgentID);
			if (AgentData)
			{
				// 각 스킬 슬롯의 어빌리티 ID 추가
				if (AgentData->AbilityID_C > 0)
				{
					AbilityIDs.Add(AgentData->AbilityID_C);
					SkillCID = AgentData->AbilityID_C;
				}
				if (AgentData->AbilityID_Q > 0)
				{
					AbilityIDs.Add(AgentData->AbilityID_Q);
					SkillQID = AgentData->AbilityID_Q;
				}
				if (AgentData->AbilityID_E > 0)
				{
					AbilityIDs.Add(AgentData->AbilityID_E);
					SkillEID = AgentData->AbilityID_E;
				}
				//if (AgentData->AbilityID_X > 0) AbilityIDs.Add(AgentData->AbilityID_X);
			}
		}
	}

	// 각 어빌리티의 스택 정보 가져와서 캐시하고 UI 업데이트
	AbilityStacksCache.Empty();
	for (int32 AbilityID : AbilityIDs)
	{
		int32 Stack = PS->GetAbilityStack(AbilityID);
		AbilityStacksCache.Add(AbilityID, Stack);
		OnAbilityStackChanged(AbilityID, Stack);
		
		// 어빌리티 정보도 함께 업데이트
		FUIAbilityInfo* AbilityInfo = AbilityInfoCache.Find(AbilityID);
		if (AbilityInfo)
		{
			AbilityInfo->CurrentStack = Stack;
		}
	}
	
	// UI 업데이트
	UpdateAbilityUI();
}

void UMatchMapShopUI::HandleAbilityStackChanged(int32 AbilityID, int32 NewStack)
{
	// 캐시 업데이트
	AbilityStacksCache.Add(AbilityID, NewStack);

	// 블루프린트 이벤트 호출
	OnAbilityStackChanged(AbilityID, NewStack);
	
	// 어빌리티 정보 캐시 업데이트
	FUIAbilityInfo* AbilityInfo = AbilityInfoCache.Find(AbilityID);
	if (AbilityInfo)
	{
		AbilityInfo->CurrentStack = NewStack;
		
		// 슬롯 타입 확인 및 UI 업데이트
		if (AbilityID == SkillCID)
		{
			if (AbilityInfo->CurrentStack == 1)
			{
				C_Stack_1->SetColorAndOpacity(PurchaseColor);
				C_Stack_2->SetColorAndOpacity(DefaultColor);
				C_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			else if (AbilityInfo->CurrentStack == 2)
			{
				C_Stack_1->SetColorAndOpacity(PurchaseColor);
				C_Stack_2->SetColorAndOpacity(PurchaseColor);
				C_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			else if (AbilityInfo->CurrentStack == 3)
			{
				C_Stack_1->SetColorAndOpacity(PurchaseColor);
				C_Stack_2->SetColorAndOpacity(PurchaseColor);
				C_Stack_3->SetColorAndOpacity(PurchaseColor);
			}
			else if (AbilityInfo->CurrentStack == 0)
			{
				C_Stack_1->SetColorAndOpacity(DefaultColor);
				C_Stack_2->SetColorAndOpacity(DefaultColor);
				C_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_C, *AbilityInfo);
		}
		else if (AbilityID == SkillQID)
		{
			if (AbilityInfo->CurrentStack == 1)
			{
				Q_Stack_1->SetColorAndOpacity(PurchaseColor);
				Q_Stack_2->SetColorAndOpacity(DefaultColor);
				Q_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			else if (AbilityInfo->CurrentStack == 2)
			{
				Q_Stack_1->SetColorAndOpacity(PurchaseColor);
				Q_Stack_2->SetColorAndOpacity(PurchaseColor);
				Q_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			else if (AbilityInfo->CurrentStack == 3)
			{
				Q_Stack_1->SetColorAndOpacity(PurchaseColor);
				Q_Stack_2->SetColorAndOpacity(PurchaseColor);
				Q_Stack_3->SetColorAndOpacity(PurchaseColor);
			}
			else if (AbilityInfo->CurrentStack == 0)
			{
				Q_Stack_1->SetColorAndOpacity(DefaultColor);
				Q_Stack_2->SetColorAndOpacity(DefaultColor);
				Q_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_Q, *AbilityInfo);
		}
		else if (AbilityID == SkillEID)
		{
			if (AbilityInfo->CurrentStack == 1)
			{
				E_Stack_1->SetColorAndOpacity(PurchaseColor);
				E_Stack_2->SetColorAndOpacity(DefaultColor);
				E_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			else if (AbilityInfo->CurrentStack == 2)
			{
				E_Stack_1->SetColorAndOpacity(PurchaseColor);
				E_Stack_2->SetColorAndOpacity(PurchaseColor);
				E_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			else if (AbilityInfo->CurrentStack == 3)
			{
				E_Stack_1->SetColorAndOpacity(PurchaseColor);
				E_Stack_2->SetColorAndOpacity(PurchaseColor);
				E_Stack_3->SetColorAndOpacity(PurchaseColor);
			}
			else if (AbilityInfo->CurrentStack == 0)
			{
				E_Stack_1->SetColorAndOpacity(DefaultColor);
				E_Stack_2->SetColorAndOpacity(DefaultColor);
				E_Stack_3->SetColorAndOpacity(DefaultColor);
			}
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_E, *AbilityInfo);
		}
	}
}

void UMatchMapShopUI::InitializeAbilityUI()
{
	if (!OwnerController)
	{
		return;
	}

	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}

	// 현재 캐릭터의 어빌리티 정보 가져오기
	int32 AgentID = PS->GetAgentID();

	if (AgentID == 0)
	{
		return;
	}

	if (!GameInstance)
	{
		GameInstance = UValorantGameInstance::Get(GetWorld());
	}

	FAgentData* AgentData = GameInstance->GetAgentData(AgentID);
	if (!AgentData)
	{
		return;
	}

	// 초기 스택 정보 업데이트
	UpdateAbilityStacks();
	
	// 어빌리티 정보 로드
	LoadAllAbilityData();
	
	// UI 업데이트
	UpdateAbilityUI();
}

// 특정 어빌리티 데이터 로드
void UMatchMapShopUI::LoadAbilityData(int32 AbilityID)
{
	if (!GameInstance)
	{
		return;
	}
	
	FAbilityData* AbilityData = GameInstance->GetAbilityData(AbilityID);
	if (!AbilityData)
	{
		return;
	}
	
	// 현재 스택 정보 가져오기
	int32 CurrentStack = GetAbilityStack(AbilityID);
	int32 MaxStack = GetMaxAbilityStack(AbilityID);
	
	// 어빌리티 정보 구조체 생성 및 캐싱
	FUIAbilityInfo AbilityInfo;
	AbilityInfo.AbilityID = AbilityID;
	AbilityInfo.AbilityName = AbilityData->AbilityName;
	AbilityInfo.ChargeCost = AbilityData->ChargeCost;
	AbilityInfo.AbilityIcon = AbilityData->AbilityIcon;
	AbilityInfo.CurrentStack = CurrentStack;
	AbilityInfo.MaxStack = MaxStack;
	
	// 캐시에 저장
	AbilityInfoCache.Add(AbilityID, AbilityInfo);
}

// 모든 어빌리티 데이터 로드
void UMatchMapShopUI::LoadAllAbilityData()
{
	if (!OwnerController)
	{
		return;
	}
	
	AAgentPlayerState* PS = OwnerController->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}
	
	// 캐릭터 ID 가져오기
	int32 AgentID = PS->GetAgentID();
	if (AgentID == 0 || !GameInstance)
	{
		return;
	}
	
	FAgentData* AgentData = GameInstance->GetAgentData(AgentID);
	if (!AgentData)
	{
		return;
	}
	
	// 각 슬롯의 어빌리티 ID 로드
	if (AgentData->AbilityID_C > 0)
	{
		SkillCID = AgentData->AbilityID_C;
		LoadAbilityData(SkillCID);
	}
	
	if (AgentData->AbilityID_Q > 0)
	{
		SkillQID = AgentData->AbilityID_Q;
		LoadAbilityData(SkillQID);
	}
	
	if (AgentData->AbilityID_E > 0)
	{
		SkillEID = AgentData->AbilityID_E;
		LoadAbilityData(SkillEID);
	}
}

// 어빌리티 UI 업데이트
void UMatchMapShopUI::UpdateAbilityUI()
{
	// 각 슬롯 별 어빌리티 정보 업데이트
	if (SkillCID > 0)
	{
		FUIAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillCID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityC_Image && AbilityInfo->AbilityIcon)
			{
				AbilityC_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}

			// Text UI 업데이트
			if (AbilityC_Text)
			{
				AbilityC_Text->SetText(FText::FromString(AbilityInfo->AbilityName));
			}
			
			// 가격 UI 업데이트
			if (AbilityC_Cost)
			{
				AbilityC_Cost->SetText(FText::FromString(FString::Printf(TEXT("%d"), AbilityInfo->ChargeCost)));
			}
			if (C_Stack_1 && C_Stack_2 && C_Stack_3)
			{
				if (AbilityInfo->MaxStack == 1)
				{
					C_Stack_1->SetVisibility(ESlateVisibility::Visible);
					C_Stack_2->SetVisibility(ESlateVisibility::Collapsed);
					C_Stack_3->SetVisibility(ESlateVisibility::Collapsed);
				}
				else if (AbilityInfo->MaxStack == 2)
				{
					C_Stack_1->SetVisibility(ESlateVisibility::Visible);
					C_Stack_2->SetVisibility(ESlateVisibility::Visible);
					C_Stack_3->SetVisibility(ESlateVisibility::Collapsed);
				}
				else if (AbilityInfo->MaxStack == 3)
				{
					C_Stack_1->SetVisibility(ESlateVisibility::Visible);
					C_Stack_2->SetVisibility(ESlateVisibility::Visible);
					C_Stack_3->SetVisibility(ESlateVisibility::Visible);
				}
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_C, *AbilityInfo);
		}
	}
	
	if (SkillQID > 0)
	{
		FUIAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillQID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityQ_Image && AbilityInfo->AbilityIcon)
			{
				AbilityQ_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}

			// Text UI 업데이트
			if (AbilityQ_Text)
			{
				AbilityQ_Text->SetText(FText::FromString(AbilityInfo->AbilityName));
			}
			
			// 가격 UI 업데이트
			if (AbilityQ_Cost)
			{
				AbilityQ_Cost->SetText(FText::FromString(FString::Printf(TEXT("%d"), AbilityInfo->ChargeCost)));
			}
			if (Q_Stack_1 && Q_Stack_2 && Q_Stack_3)
			{
				if (AbilityInfo->MaxStack == 1)
				{
					Q_Stack_1->SetVisibility(ESlateVisibility::Visible);
					Q_Stack_2->SetVisibility(ESlateVisibility::Collapsed);
					Q_Stack_3->SetVisibility(ESlateVisibility::Collapsed);
				}
				else if (AbilityInfo->MaxStack == 2)
				{
					Q_Stack_1->SetVisibility(ESlateVisibility::Visible);
					Q_Stack_2->SetVisibility(ESlateVisibility::Visible);
					Q_Stack_3->SetVisibility(ESlateVisibility::Collapsed);
				}
				else if (AbilityInfo->MaxStack == 3)
				{
					Q_Stack_1->SetVisibility(ESlateVisibility::Visible);
					Q_Stack_2->SetVisibility(ESlateVisibility::Visible);
					Q_Stack_3->SetVisibility(ESlateVisibility::Visible);
				}
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_Q, *AbilityInfo);
		}
	}
	
	if (SkillEID > 0)
	{
		FUIAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillEID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityE_Image && AbilityInfo->AbilityIcon)
			{
				AbilityE_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}

			// Text UI 업데이트
			if (AbilityE_Text)
			{
				AbilityE_Text->SetText(FText::FromString(AbilityInfo->AbilityName));
			}
			
			// 가격 UI 업데이트
			if (AbilityE_Cost)
			{
				AbilityE_Cost->SetText(FText::FromString(FString::Printf(TEXT("%d"), AbilityInfo->ChargeCost)));
			}
			if (E_Stack_1 && E_Stack_2 && E_Stack_3)
			{
				if (AbilityInfo->MaxStack == 1)
				{
					E_Stack_1->SetVisibility(ESlateVisibility::Visible);
					E_Stack_2->SetVisibility(ESlateVisibility::Collapsed);
					E_Stack_3->SetVisibility(ESlateVisibility::Collapsed);
				}
				else if (AbilityInfo->MaxStack == 2)
				{
					E_Stack_1->SetVisibility(ESlateVisibility::Visible);
					E_Stack_2->SetVisibility(ESlateVisibility::Visible);
					E_Stack_3->SetVisibility(ESlateVisibility::Collapsed);
				}
				else if (AbilityInfo->MaxStack == 3)
				{
					E_Stack_1->SetVisibility(ESlateVisibility::Visible);
					E_Stack_2->SetVisibility(ESlateVisibility::Visible);
					E_Stack_3->SetVisibility(ESlateVisibility::Visible);
				}
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_E, *AbilityInfo);
		}
	}
}

// 어빌리티 정보 가져오기
FUIAbilityInfo UMatchMapShopUI::GetAbilityInfo(int32 AbilityID) const
{
	const FUIAbilityInfo* InfoPtr = AbilityInfoCache.Find(AbilityID);
	if (InfoPtr)
	{
		return *InfoPtr;
	}
	
	// 기본 빈 정보 반환
	return FUIAbilityInfo();
}

// 슬롯별 어빌리티 정보 가져오기
FUIAbilityInfo UMatchMapShopUI::GetAbilityInfoBySlot(EAbilitySlotType SlotType) const
{
	int32 AbilityID = 0;
	
	switch (SlotType)
	{
	case EAbilitySlotType::Slot_C:
		AbilityID = SkillCID;
		break;
	case EAbilitySlotType::Slot_Q:
		AbilityID = SkillQID;
		break;
	case EAbilitySlotType::Slot_E:
		AbilityID = SkillEID;
		break;
	default:
		break;
	}
	
	if (AbilityID > 0)
	{
		return GetAbilityInfo(AbilityID);
	}
	
	// 기본 빈 정보 반환
	return FUIAbilityInfo();
}

// 색상 변경 로직을 통합한 함수 구현
void UMatchMapShopUI::FlashCreditTextColor(const FLinearColor& FlashColor, float Duration)
{
	if (!CreditText)
	{
		return;
	}

	// 이미 타이머가 작동 중이면 중지
	if (GetWorld()->GetTimerManager().IsTimerActive(CreditTextColorTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(CreditTextColorTimerHandle);
	}

	// 색상 변경
	CreditText->SetColorAndOpacity(FSlateColor(FlashColor));

	// 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		CreditTextColorTimerHandle,
		this,
		&UMatchMapShopUI::ResetCreditTextColor,
		Duration,
		false);
}

// 원래 색상으로 복원하는 함수
void UMatchMapShopUI::ResetCreditTextColor()
{
	if (CreditText)
	{
		CreditText->SetColorAndOpacity(OriginalCreditTextColor);
	}
}

// 어빌리티 비주얼 상태 가져오기
EAbilityVisualState UMatchMapShopUI::GetAbilityVisualState(int32 AbilityID) const
{
    int32 CurrentStack = GetAbilityStack(AbilityID);
    int32 MaxStack = GetMaxAbilityStack(AbilityID);
    
    if (CurrentStack == 0)
    {
        return EAbilityVisualState::Empty;
    }
    else if (CurrentStack < MaxStack)
    {
        return EAbilityVisualState::Partial;
    }
    else
    {
        return EAbilityVisualState::Full;
    }
}

// 슬롯별 어빌리티 비주얼 상태 가져오기
EAbilityVisualState UMatchMapShopUI::GetAbilityVisualStateBySlot(EAbilitySlotType SlotType) const
{
    int32 AbilityID = 0;
    
    switch (SlotType)
    {
    case EAbilitySlotType::Slot_C:
        AbilityID = SkillCID;
        break;
    case EAbilitySlotType::Slot_Q:
        AbilityID = SkillQID;
        break;
    case EAbilitySlotType::Slot_E:
        AbilityID = SkillEID;
        break;
    default:
        break;
    }
    
    if (AbilityID > 0)
    {
        return GetAbilityVisualState(AbilityID);
    }
    
    return EAbilityVisualState::Empty;
}