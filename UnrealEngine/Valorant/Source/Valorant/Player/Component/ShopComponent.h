// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager/ValorantGameType.h"
#include "ShopComponent.generated.h"


class AAgentPlayerController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UShopComponent();

	// ToDo : 데이터 기반 무기 - 스킬 초기화 로직
	void InitBySkillData(TArray<int32> SkillIDs);

	// 상점에서 판매하는 스킬 목록 - ToDo 초기화 필요
	TMap<int32, FAbilityData*> AvailableAbilities;

	// 스킬 구매 함수
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool PurchaseAbility(int32 AbilityID);

protected:
	virtual void BeginPlay() override;

private:
	// 상점 소유자
	UPROPERTY()
	AAgentPlayerController* m_Owner;
};
