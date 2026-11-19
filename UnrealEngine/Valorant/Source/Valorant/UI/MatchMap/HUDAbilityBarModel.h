#pragma once

#include "CoreMinimal.h"
#include "ResourceManager/ValorantGameType.h"
#include "HUDAbilityBarModel.generated.h"

class AAgentPlayerState;
class UValorantGameInstance;

// 스킬 슬롯별 스택 정보 구조체
USTRUCT(BlueprintType)
struct FAbilitySlotStackInfo
{
	GENERATED_BODY()

	// 현재 스택 수
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStack = 0;

	// 최대 스택 수
	UPROPERTY(BlueprintReadOnly)
	int32 MaxStack = 0;

	// 어빌리티 ID
	UPROPERTY(BlueprintReadOnly)
	int32 AbilityID = 0;

	// 초기화 함수
	void Initialize(int32 InAbilityID, int32 InCurrentStack, int32 InMaxStack)
	{
		AbilityID = InAbilityID;
		CurrentStack = InCurrentStack;
		MaxStack = InMaxStack;
	}
};

// 어빌리티 상세 정보 구조체 (HUD용)
USTRUCT(BlueprintType)
struct FHUDAbilityInfo
{
	GENERATED_BODY()

	// 어빌리티 ID
	UPROPERTY(BlueprintReadOnly)
	int32 AbilityID = 0;

	// 어빌리티 이름
	UPROPERTY(BlueprintReadOnly)
	FString AbilityName = "";

	// 어빌리티 설명
	UPROPERTY(BlueprintReadOnly)
	FText AbilityDescription;

	// 어빌리티 아이콘
	UPROPERTY(BlueprintReadOnly)
	UTexture2D* AbilityIcon = nullptr;

	// 현재 스택 수
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStack = 0;

	// 최대 스택 수
	UPROPERTY(BlueprintReadOnly)
	int32 MaxStack = 0;

	// 충전 비용
	UPROPERTY(BlueprintReadOnly)
	int32 ChargeCost = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FHUDAbilitySlotChanged, EAbilitySlotType);

UCLASS()
class UHUDAbilityBarModel : public UObject
{
    GENERATED_BODY()

public:
    static constexpr EAbilitySlotType SlotTypes[] = {
        EAbilitySlotType::Slot_C, EAbilitySlotType::Slot_Q, EAbilitySlotType::Slot_E, EAbilitySlotType::Slot_X
    };

    bool Bind(AAgentPlayerState* PlayerState, UValorantGameInstance* GameInstance);
    void Unbind();
    FHUDAbilityInfo GetSlot(EAbilitySlotType Slot) const;
    FHUDAbilityInfo GetAbility(int32 AbilityID) const;
    int32 GetCurrentStack(int32 AbilityID) const;
    int32 GetMaxStack(int32 AbilityID) const;
    void RefreshSlot(EAbilitySlotType Slot, int32 AbilityID);

    FHUDAbilitySlotChanged OnSlotChanged;

private:
    UFUNCTION()
    void OnStackChanged(int32 AbilityID, int32 NewStack);

    TWeakObjectPtr<AAgentPlayerState> BoundPlayerState;
    int32 BoundAgentId = INDEX_NONE;

    UPROPERTY(Transient)
    TMap<EAbilitySlotType, FHUDAbilityInfo> Slots;
};
