#include "CombatHUDViewModel.h"

#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Player/Component/CreditComponent.h"
#include "Weapon/BaseWeapon.h"

void UCombatHUDViewModel::Bind(AAgentPlayerController& Controller)
{
    const auto* playerState = Controller.GetPlayerState<AAgentPlayerState>();
    auto* attributes = playerState ? playerState->GetBaseAttributeSet() : nullptr;
    auto* credits = playerState ? playerState->GetCreditComponent() : nullptr;
    const bool bSourcesChanged = BoundController.Get() != &Controller
        || Attributes.Get() != attributes || CreditSource.Get() != credits;
    if (bSourcesChanged)
    {
        Unbind();
        BoundController = &Controller;
        Attributes = attributes;
        CreditSource = credits;
        Controller.OnChangedAmmo.AddUniqueDynamic(this, &UCombatHUDViewModel::OnAmmoChanged);
        if (attributes)
        {
            attributes->OnHealthChanged.AddUniqueDynamic(this, &UCombatHUDViewModel::OnHealthChanged);
            attributes->OnMaxHealthChanged.AddUniqueDynamic(this, &UCombatHUDViewModel::OnMaxHealthChanged);
            attributes->OnArmorChanged.AddUniqueDynamic(this, &UCombatHUDViewModel::OnArmorChanged);
        }
        if (credits)
        {
            credits->OnCreditChanged.AddUniqueDynamic(this, &UCombatHUDViewModel::OnCreditChanged);
        }
        ReadVitals();
        OnCreditChanged(credits ? credits->GetCurrentCredit() : 0);
    }

    auto* pawn = Controller.GetPawn<ABaseAgent>();
    if (bSourcesChanged || BoundPawn.Get() != pawn || BoundPawn.IsStale())
    {
        if (auto* previous = BoundPawn.Get())
        {
            previous->OnActiveInteractorChanged.RemoveAll(this);
        }
        BoundPawn = pawn;
        if (pawn)
        {
            pawn->OnActiveInteractorChanged.AddUObject(this, &UCombatHUDViewModel::ReadActiveAmmo);
        }
        ReadActiveAmmo();
    }
}

void UCombatHUDViewModel::Unbind()
{
    if (auto* controller = BoundController.Get())
    {
        controller->OnChangedAmmo.RemoveDynamic(this, &UCombatHUDViewModel::OnAmmoChanged);
    }
    if (auto* attributes = Attributes.Get())
    {
        attributes->OnHealthChanged.RemoveDynamic(this, &UCombatHUDViewModel::OnHealthChanged);
        attributes->OnMaxHealthChanged.RemoveDynamic(this, &UCombatHUDViewModel::OnMaxHealthChanged);
        attributes->OnArmorChanged.RemoveDynamic(this, &UCombatHUDViewModel::OnArmorChanged);
    }
    if (auto* credits = CreditSource.Get())
    {
        credits->OnCreditChanged.RemoveDynamic(this, &UCombatHUDViewModel::OnCreditChanged);
    }
    if (auto* pawn = BoundPawn.Get())
    {
        pawn->OnActiveInteractorChanged.RemoveAll(this);
    }
    BoundController.Reset();
    Attributes.Reset();
    CreditSource.Reset();
    BoundPawn.Reset();
}

void UCombatHUDViewModel::ReadActiveAmmo()
{
    const auto* pawn = BoundPawn.Get();
    const auto* weapon = pawn ? Cast<ABaseWeapon>(pawn->GetCurrentInterator()) : nullptr;
    const auto type = IsValid(weapon) ? weapon->GetInteractorType() : EInteractorType::None;
    const bool bHasGun = type == EInteractorType::MainWeapon || type == EInteractorType::SubWeapon;
    OnAmmoChanged(bHasGun, bHasGun ? weapon->MagazineAmmo : 0, bHasGun ? weapon->SpareAmmo : 0);
}

void UCombatHUDViewModel::ReadVitals()
{
    FHUDVitals values;
    if (const auto* attributes = Attributes.Get())
    {
        values.Health = FMath::TruncToInt(attributes->GetHealth());
        values.MaxHealth = FMath::TruncToInt(attributes->GetMaxHealth());
        values.Armor = FMath::TruncToInt(attributes->GetArmor());
    }
    UE_MVVM_SET_PROPERTY_VALUE(Vitals, values);
}

void UCombatHUDViewModel::OnHealthChanged(float Health, bool bIsDamage)
{
    ReadVitals();
}

void UCombatHUDViewModel::OnMaxHealthChanged(float MaxHealth)
{
    ReadVitals();
}

void UCombatHUDViewModel::OnArmorChanged(float Armor)
{
    ReadVitals();
}

void UCombatHUDViewModel::OnAmmoChanged(bool bVisible, int32 Magazine, int32 Reserve)
{
    FHUDAmmo values;
    if (bVisible)
    {
        values.bVisible = true;
        values.Magazine = Magazine;
        values.Reserve = Reserve;
    }
    UE_MVVM_SET_PROPERTY_VALUE(Ammo, values);
}

void UCombatHUDViewModel::OnCreditChanged(int32 NewCredits)
{
    UE_MVVM_SET_PROPERTY_VALUE(Credits, NewCredits);
}
