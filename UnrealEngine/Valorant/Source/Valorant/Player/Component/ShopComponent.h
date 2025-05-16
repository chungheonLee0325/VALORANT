// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager/ValorantGameType.h"
#include "ShopComponent.generated.h"

class UCreditComponent;
class AAgentPlayerController;
class UValorantGameInstance;
class ABaseWeapon;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopAvailabilityChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedWeaponsChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UShopComponent();

	// 상점 상태 관리 (이제 UI 열기/닫기 로직을 포함하지 않음)
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetShopActive(bool bActive);

	// 상점 상태 확인
	UFUNCTION(BlueprintPure, Category = "Shop")
	bool IsShopActive() const { return bIsShopActive; }

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
	bool CanPurchaseItem(int32 ItemID, EShopItemType ItemType, int32& OutRefundAmount) const;

	// 구매 이벤트 
	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopItemPurchased OnShopItemPurchased;

	// 상점 가용성 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopAvailabilityChanged OnShopAvailabilityChanged;

	// 상점에서 판매하는 아이템 목록
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TArray<FShopItem> ShopItems;

	// 특정 타입의 아이템만 가져오기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	TArray<FShopItem> GetShopItemsByType(EShopItemType ItemType) const;

	// 모든 상점 아이템 가져오기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	const TArray<FShopItem>& GetAllShopItems() const { return ShopItems; }

	// 크레딧 동기화 요청
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestCreditSync();

	// 현재 보유 중인 무기 ID 배열 반환
	UFUNCTION(BlueprintCallable, Category = "Shop")
	TArray<int32> GetEquippedWeaponIDs() const;

	// 특정 무기를 보유 중인지 확인
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool IsWeaponEquipped(int32 WeaponID) const;

	// 어빌리티 스택 정보 가져오기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	int32 GetAbilityStack(int32 AbilityID) const;

	// 어빌리티의 최대 스택 수 가져오기
	UFUNCTION(BlueprintCallable, Category = "Shop")
	int32 GetMaxAbilityStack(int32 AbilityID) const;

	// 무기 장착 상태 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnEquippedWeaponsChanged OnEquippedWeaponsChanged;

protected:
	virtual void BeginPlay() override;

	// 아이템 ID로 상점 아이템 찾기
	FShopItem* FindShopItem(int32 ItemID, EShopItemType ItemType);

	// 라운드 상태 확인 (상점이 이용 가능한지)
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

	// 무기 생성 및 플레이어 할당
	// Sidearm은 SecondWeapon, 다른 무기는 PrimaryWeapon에 할당
	void SpawnWeaponForPlayer(int32 WeaponID);

	// 상점 활성화 상태
	UPROPERTY(BlueprintReadOnly, Category = "Shop", meta = (AllowPrivateAccess = "true"))
	bool bIsShopActive;
};
