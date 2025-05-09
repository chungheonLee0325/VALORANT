// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapShopUI.h"
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/Component/ShopComponent.h"
#include "Valorant/Player/Component/CreditComponent.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "Components/TextBlock.h"

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
		//RequestLatestCreditValue();
		//UpdateCreditDisplay(0);
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
		
		// 크레딧 컴포넌트의 델리게이트에도 바인딩 (이중 안전장치)
		UCreditComponent* CreditComp = PS->GetCreditComponent();
		if (CreditComp)
		{
			// 서버에 최신 크레딧 값 요청 (명시적 동기화)
			RequestLatestCreditValue();
			
			// 크레딧 변경 이벤트에 바인딩
			CreditComp->OnCreditChanged.AddDynamic(this, &UMatchMapShopUI::UpdateCreditDisplay);
		}
	}
	
	// 상점 아이템 목록 업데이트
	UpdateShopItemList();

	// 상점 컴포넌트의 무기 장착 변경 이벤트에 연결
	if (Controller && Controller->ShopComponent)
	{
		Controller->ShopComponent->OnEquippedWeaponsChanged.AddDynamic(this, &UMatchMapShopUI::UpdateWeaponHighlights);
        
		// 초기 하이라이트 상태 업데이트
		UpdateWeaponHighlights();
	}

	// 서버로부터 오는 구매 결과 델리게이트에 바인딩
	if (OwnerController)
	{
		OwnerController->OnServerPurchaseResult.AddDynamic(this, &UMatchMapShopUI::HandleServerPurchaseResult);
	}
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
			
			// 가격 확인 및 크레딧 충분한지 검사
			if (Item && !CreditComp->CanUseCredits(Item->Price))
			{
				// 크레딧 부족 시 UI 피드백 (빨간색으로 크레딧 텍스트 깜박임 등)
				if (CreditText)
				{
					// 기존 색상 저장
					FSlateColor OriginalColor = CreditText->GetColorAndOpacity();
					
					// 빨간색으로 변경
					CreditText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
					
					// 타이머로 원래 색상으로 되돌리기
					FTimerHandle TimerHandle;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, OriginalColor]() {
						if (CreditText)
						{
							CreditText->SetColorAndOpacity(OriginalColor);
						}
					}, 0.5f, false);
				}
				
				// 구매 실패 처리
				HandleServerPurchaseResult(false, WeaponId, EShopItemType::Weapon, TEXT(""));
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

	// ShopComponent를 통해 스킬 구매 요청
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		ShopComp->PurchaseAbility(SkillId);
		// 구매 결과 처리 -> 서버 응답으로 대체
	}
	else
	{
		// HandlePurchaseResult(false, SkillId, EShopItemType::Ability);
	}
}

void UMatchMapShopUI::OnClickedBuyShiledButton(const int ShieldId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		return;
	}

	// ShopComponent를 통해 방어구 구매 요청
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		ShopComp->PurchaseArmor(ShieldId);
		// 구매 결과 처리 -> 서버 응답으로 대체
	}
	else
	{
		// HandlePurchaseResult(false, ShieldId, EShopItemType::Armor);
	}
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

void UMatchMapShopUI::HandleServerPurchaseResult(bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason)
{
	UE_LOG(LogTemp, Log, TEXT("HandleServerPurchaseResult: Success: %d, ItemID: %d, Type: %s, Reason: %s"), bSuccess, ItemID, *UEnum::GetValueAsString(ItemType), *FailureReason);

	// 구매 결과 이벤트 발생 (Blueprint 등에서 사용 가능)
	OnPurchaseResult.Broadcast(bSuccess, ItemID, ItemType);

	if (bSuccess)
	{
		UpdateShopItemList(); // 아이템 목록 UI 갱신
		// 크레딧 표시는 OnCreditChanged 델리게이트를 통해 자동으로 업데이트될 것이므로, 여기서 직접 호출할 필요는 없을 수 있음
		// 필요하다면 RequestLatestCreditValue(); 호출 고려
		UE_LOG(LogTemp, Log, TEXT("Purchase successful for Item %d (%s)"), ItemID, *UEnum::GetValueAsString(ItemType));
		// 여기에 성공 시 UI 피드백 로직 추가 (예: 성공 메시지, 사운드)
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Purchase failed for Item %d (%s). Reason: %s"), ItemID, *UEnum::GetValueAsString(ItemType), *FailureReason);
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
    
	// 현재 장착 중인 무기 ID 가져오기
	EquippedWeaponIDs = OwnerController->ShopComponent->GetEquippedWeaponIDs();
    
	// Blueprint에서 구현할 UI 업데이트 이벤트 호출
	OnWeaponHighlightUpdated();
}

bool UMatchMapShopUI::IsWeaponEquipped(int32 WeaponID) const
{
	return EquippedWeaponIDs.Contains(WeaponID);
}
