#pragma once

#include "MVVMViewModelBase.h"
#include "CombatHUDViewModel.generated.h"

class AAgentPlayerController;
class ABaseAgent;
class UBaseAttributeSet;
class UCreditComponent;

USTRUCT(BlueprintType)
struct FHUDVitals
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 Health = 0;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 MaxHealth = 0;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 Armor = 0;

    bool operator==(const FHUDVitals& Other) const = default;
};

USTRUCT(BlueprintType)
struct FHUDAmmo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    bool bVisible = false;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 Magazine = 0;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 Reserve = 0;

    bool operator==(const FHUDAmmo& Other) const = default;
};

UCLASS()
class UCombatHUDViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    void Bind(AAgentPlayerController& Controller);
    void Unbind();

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "HUD")
    FHUDVitals Vitals;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "HUD")
    FHUDAmmo Ammo;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "HUD")
    int32 Credits = 0;

private:
    UFUNCTION()
    void OnHealthChanged(float Health, bool bIsDamage);

    UFUNCTION()
    void OnMaxHealthChanged(float MaxHealth);

    UFUNCTION()
    void OnArmorChanged(float Armor);

    UFUNCTION()
    void OnAmmoChanged(bool bVisible, int32 Magazine, int32 Reserve);

    UFUNCTION()
    void OnCreditChanged(int32 NewCredits);

    void ReadVitals();
    void ReadActiveAmmo();

    TWeakObjectPtr<AAgentPlayerController> BoundController;
    TWeakObjectPtr<UBaseAttributeSet> Attributes;
    TWeakObjectPtr<UCreditComponent> CreditSource;
    TWeakObjectPtr<ABaseAgent> BoundPawn;
};
