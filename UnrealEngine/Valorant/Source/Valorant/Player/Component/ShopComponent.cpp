// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopComponent.h"

#include "Player/Agent/BaseAgent.h"


UShopComponent::UShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UShopComponent::BeginPlay()
{
	Super::BeginPlay();

	m_Owner = Cast<AAgentPlayerController>(GetOwner());
}

void UShopComponent::InitBySkillData(TArray<int32> SkillIDs)
{
}

bool UShopComponent::PurchaseAbility(int32 AbilityID)
{
	if (!m_Owner || AbilityID < 0 || AbilityID >= AvailableAbilities.Num())
	{
		return false;
	}

	// 해당 스킬 정보 가져오기
	FAbilityData** AbilityInfo = AvailableAbilities.Find(AbilityID);

	if (AbilityInfo != nullptr && *AbilityInfo != nullptr)
	{
		// 캐릭터에 스킬 구매 요청
		m_Owner->RequestPurchaseAbility(AbilityID);
		return true;
	}
	return false;
}
