// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapShopUI.h"
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/Component/ShopComponent.h"
#include "Valorant/Player/Component/CreditComponent.h"
#include "Valorant/Player/AgentPlayerState.h"

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
		UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
		if (CreditComp)
		{
			UpdateCreditDisplay(CreditComp->GetCurrentCredit());
			
			// 크레딧 변경 이벤트에 바인딩
			CreditComp->OnCreditChanged.AddDynamic(this, &UMatchMapShopUI::UpdateCreditDisplay);
		}
	}
}

void UMatchMapShopUI::UpdateCreditDisplay(int32 CurrentCredit)
{
	// 블루프린트에서 구현할 수 있도록 제공하는 함수
	// 크레딧 텍스트 업데이트 로직은 BP에서 처리
}

void UMatchMapShopUI::OnClickedBuyWeaponButton(const int WeaponId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		HandlePurchaseResult(false, WeaponId, EShopItemType::Weapon);
		return;
	}
	OwnerController->RequestPurchaseWeapon(WeaponId);
	
	// // ShopComponent를 통해 무기 구매 요청
	// UShopComponent* ShopComp = OwnerController->ShopComponent;
	// if (ShopComp)
	// {
	// 	bool bSuccess = ShopComp->PurchaseWeapon(WeaponId);
	// 	
	// 	// 구매 결과 처리
	// 	HandlePurchaseResult(bSuccess, WeaponId, EShopItemType::Weapon);
	// }
	// else
	// {
	// 	HandlePurchaseResult(false, WeaponId, EShopItemType::Weapon);
	// }
}

void UMatchMapShopUI::OnClickedBuySkillButton(const int SkillId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		HandlePurchaseResult(false, SkillId, EShopItemType::Ability);
		return;
	}

	// ShopComponent를 통해 스킬 구매 요청
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		bool bSuccess = ShopComp->PurchaseAbility(SkillId);
		
		// 구매 결과 처리
		HandlePurchaseResult(bSuccess, SkillId, EShopItemType::Ability);
	}
	else
	{
		HandlePurchaseResult(false, SkillId, EShopItemType::Ability);
	}
}

void UMatchMapShopUI::OnClickedBuyShiledButton(const int ShieldId)
{
	if (!OwnerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUI: 컨트롤러가 없습니다!"));
		HandlePurchaseResult(false, ShieldId, EShopItemType::Armor);
		return;
	}

	// ShopComponent를 통해 방어구 구매 요청
	UShopComponent* ShopComp = OwnerController->ShopComponent;
	if (ShopComp)
	{
		bool bSuccess = ShopComp->PurchaseArmor(ShieldId);
		
		// 구매 결과 처리
		HandlePurchaseResult(bSuccess, ShieldId, EShopItemType::Armor);
	}
	else
	{
		HandlePurchaseResult(false, ShieldId, EShopItemType::Armor);
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

void UMatchMapShopUI::HandlePurchaseResult(bool bSuccess, int32 ItemID, EShopItemType ItemType)
{
	// 구매 결과 이벤트 발생
	OnPurchaseResult.Broadcast(bSuccess, ItemID, ItemType);
	
	// 성공한 경우 아이템 목록 업데이트
	if (bSuccess)
	{
		UpdateShopItemList();
	}
}