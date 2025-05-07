// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager/ValorantGameType.h"
#include "ShopComponent.generated.h"

class UCreditComponent;
class AAgentPlayerController;
class UValorantGameInstance;

// 상점에서 구매 가능한 아이템 종류
UENUM(BlueprintType)
enum class EShopItemType : uint8
{
	None UMETA(DisplayName = "None"),
	Weapon UMETA(DisplayName = "Weapon"),
	Ability UMETA(DisplayName = "Ability"),
	Armor UMETA(DisplayName = "Armor"),
};

// 상점 아이템 구조체
USTRUCT(BlueprintType)
struct FShopItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EShopItemType ItemType = EShopItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* ItemIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UObject> ItemClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsAvailable = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopItemPurchased, const FShopItem&, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UShopComponent();

	// 상점 UI 열기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShop();

	// 상점 UI 닫기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void CloseShop();

	// 상점 아이템 초기화
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void InitializeShopItems();

	// 능력 초기화
	void InitBySkillData(TArray<int32> SkillIDs);

	// 무기 구매 함수
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool PurchaseWeapon(int32 WeaponID);

	// 능력 구매 함수
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool PurchaseAbility(int32 AbilityID);

	// 방어구 구매 함수
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool PurchaseArmor(int32 ArmorLevel);

	// 구매 가능 여부 확인
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool CanPurchaseItem(int32 ItemID, EShopItemType ItemType) const;

	// 구매 이벤트 
	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopItemPurchased OnShopItemPurchased;

	// 상점에서 판매하는 아이템 목록
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TArray<FShopItem> ShopItems;

	// 특정 타입의 아이템만 가져오기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	TArray<FShopItem> GetShopItemsByType(EShopItemType ItemType) const;

	// 모든 상점 아이템 가져오기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	const TArray<FShopItem>& GetAllShopItems() const { return ShopItems; }

	// 상점 열기/닫기 상태
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	bool bIsShopOpen;

protected:
	virtual void BeginPlay() override;

	// 아이템 ID로 상점 아이템 찾기
	FShopItem* FindShopItem(int32 ItemID, EShopItemType ItemType);

	// 라운드 상태 확인 (상점이 열려있는지)
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool IsShopAvailable() const;

private:
	// 상점 소유자
	UPROPERTY()
	AAgentPlayerController* m_Owner;

	// 게임 인스턴스 참조
	UPROPERTY()
	UValorantGameInstance* GameInstance;

	// 구매 가능한 무기 목록
	TMap<int32, FWeaponData*> AvailableWeapons;

	// 구매 가능한 능력 목록
	TMap<int32, FAbilityData*> AvailableAbilities;
};
