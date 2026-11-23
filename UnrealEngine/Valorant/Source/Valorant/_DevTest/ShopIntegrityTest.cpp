#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "EngineUtils.h"
#include "GameManager/ValorantGameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Player/Component/CreditComponent.h"
#include "Player/Component/ShopComponent.h"
#include "Weapon/BaseWeapon.h"
#include "Weapon/ThirdPersonInteractor.h"
#include "UIRegressionEnvironment.h"

namespace
{
    void Record(const TCHAR* ID, const TCHAR* Check, bool bPass)
    {
        if (bPass)
        {
            UE_LOG(LogTemp, Display, TEXT("SHOP CHECK PASS [%s] %s"), ID, Check);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SHOP CHECK FAIL [%s] %s"), ID, Check);
        }
    }

    void CheckArmor(UShopComponent& Shop, UCreditComponent& Credits, UBaseAttributeSet& Attributes)
    {
        Credits.AddCredits(5000 - Credits.GetCurrentCredit());
        Attributes.SetArmor(0.f);
        const bool bFirstLight = Shop.PurchaseArmor(1) && Attributes.GetArmor() == 25.f
            && Credits.GetCurrentCredit() == 4600;
        int32 refund = 0;
        const bool bDuplicate = !Shop.CanPurchaseItem(1, EShopItemType::Armor, refund)
            && !Shop.PurchaseArmor(1) && !Shop.PurchaseArmor(1) && Credits.GetCurrentCredit() == 4600;
        Record(TEXT("SHOP.ARMOR.DUPLICATE"), TEXT("armor: initial purchase and repeated full-tier rejection"), bFirstLight && bDuplicate);

        Attributes.SetArmor(10.f);
        const bool bRefill = Shop.PurchaseArmor(1) && Attributes.GetArmor() == 25.f && Credits.GetCurrentCredit() == 4200;
        const bool bUpgrade = Shop.PurchaseArmor(2) && Attributes.GetArmor() == 50.f && Credits.GetCurrentCredit() == 3200;
        Attributes.SetArmor(10.f);
        const bool bDowngrade = !Shop.CanPurchaseItem(1, EShopItemType::Armor, refund)
            && !Shop.PurchaseArmor(1) && Attributes.GetArmor() == 10.f && Credits.GetCurrentCredit() == 3200;
        Record(TEXT("SHOP.ARMOR.DAMAGED_UPGRADE"), TEXT("armor: damaged refill, full-price upgrade, damaged downgrade rejection"), bRefill && bUpgrade && bDowngrade);

        Credits.AddCredits(100 - Credits.GetCurrentCredit());
        Record(TEXT("SHOP.ARMOR.INVALID"), TEXT("armor: insufficient credit and unknown ID preserve balance and armor"),
            !Shop.PurchaseArmor(2) && !Shop.PurchaseArmor(3) && Credits.GetCurrentCredit() == 100 && Attributes.GetArmor() == 10.f);
    }

    TPair<int32, int32> CountWeaponActors(UWorld& World)
    {
        TPair<int32, int32> counts(0, 0);
        for (TActorIterator<ABaseInteractor> actor(&World); actor; ++actor)
        {
            counts.Key += actor->IsA<ABaseWeapon>() ? 1 : 0;
        }
        for (TActorIterator<AThirdPersonInteractor> actor(&World); actor; ++actor)
        {
            ++counts.Value;
        }
        return counts;
    }

    bool CheckRequestOrder(AAgentPlayerController& PC, UCreditComponent& Credits, int32 WeaponID)
    {
        auto& shop = *PC.ShopComponent;
        const uint32 older = PC.AllocateShopRequestID();
        const uint32 purchase = PC.AllocateShopRequestID();
        int32 rejections = 0;
        const auto replies = PC.OnShopPurchaseResponse.AddLambda(
            [&](uint32 ID, bool bSuccess, int32, EShopItemType, const FString& Reason)
            {
                rejections += (ID == purchase || ID == older) && !bSuccess && Reason.Contains(TEXT("이미 처리"));
            });
        const bool bBought = shop.PurchaseItem(WeaponID, EShopItemType::Weapon, purchase);
        const int32 balance = Credits.GetCurrentCredit();
        const bool bReplayRejected = !shop.PurchaseItem(WeaponID, EShopItemType::Weapon, purchase);
        // CheckArmor leaves a damaged heavy shield, so a new number would refill it.
        const bool bOlderRejected = !shop.PurchaseItem(2, EShopItemType::Armor, older);
        PC.OnShopPurchaseResponse.Remove(replies);
        UE_LOG(LogTemp, Display, TEXT("SHOP CHECK request order: bought=%d rejections=%d balance=%d/%d"),
            bBought, rejections, Credits.GetCurrentCredit(), balance);
        Record(TEXT("SHOP.REQUEST.ORDER"), TEXT("requests: a processed or older request number is rejected without settlement"),
            bBought && bReplayRejected && bOlderRejected && rejections == 2 && Credits.GetCurrentCredit() == balance);
        return bBought;
    }

    void CheckWorldPickup(ABaseAgent& Agent, ABaseWeapon& WorldWeapon)
    {
        auto* mesh = WorldWeapon.GetMesh();
        auto* previous = Agent.GetMainWeapon();
        auto* world = Agent.GetWorld();
        const FTransform transform = WorldWeapon.GetActorTransform();
        const auto counts = CountWeaponActors(*world);
        bool bExpectedPhysics = mesh->IsSimulatingPhysics();
        const auto collision = mesh->GetCollisionEnabled();
        const auto unchanged = [&]()
        {
            const bool bPreserved = Agent.GetMainWeapon() == previous && previous && previous->GetOwnerAgent() == &Agent
                && !WorldWeapon.GetOwnerAgent() && !WorldWeapon.GetOwner()
                && !mesh->GetAttachParent() && WorldWeapon.GetActorTransform().Equals(transform, 0.1)
                && mesh->IsSimulatingPhysics() == bExpectedPhysics && mesh->GetCollisionEnabled() == collision
                && CountWeaponActors(*world) == counts;
            if (!bPreserved)
            {
                UE_LOG(LogTemp, Display, TEXT("PICKUP state slot=%d owner=%s parent=%s transform=%d physics=%d/%d collision=%d/%d actors=%d/%d expected=%d/%d"),
                    Agent.GetMainWeapon() == previous, *GetNameSafe(WorldWeapon.GetOwnerAgent()), *GetNameSafe(mesh->GetAttachParent()),
                    WorldWeapon.GetActorTransform().Equals(transform, 0.1), mesh->IsSimulatingPhysics(), bExpectedPhysics,
                    static_cast<int32>(mesh->GetCollisionEnabled()), static_cast<int32>(collision),
                    CountWeaponActors(*world).Key, CountWeaponActors(*world).Value, counts.Key, counts.Value);
            }
            return bPreserved;
        };

        const bool bOriginalPhysics = bExpectedPhysics;
        mesh->SetMobility(EComponentMobility::Static);
        bExpectedPhysics = mesh->IsSimulatingPhysics();
        Agent.ServerRPC_Interact(&WorldWeapon);
        const bool bFirstPersonPreserved = unchanged();
        mesh->SetMobility(EComponentMobility::Movable);
        mesh->SetSimulatePhysics(bOriginalPhysics);
        bExpectedPhysics = bOriginalPhysics;
        Record(TEXT("WEAPON.PICKUP.FIRST_PERSON_FAILURE"), TEXT("world pickup: first-person attachment failure preserves the world weapon and previous slot"), bFirstPersonPreserved);

        {
            UClass* visualClass = AThirdPersonInteractor::StaticClass();
            TGuardValue<EClassFlags> abstractVisual(visualClass->ClassFlags, visualClass->ClassFlags | CLASS_Abstract);
            Agent.ServerRPC_Interact(&WorldWeapon);
        }
        Record(TEXT("WEAPON.PICKUP.SPAWN_FAILURE"), TEXT("world pickup: presentation spawn failure restores transform physics collision and ownership"), unchanged());

        const auto spawn = world->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([](AActor* Actor)
        {
            if (auto* visual = Cast<AThirdPersonInteractor>(Actor))
            {
                visual->GetRootComponent()->SetMobility(EComponentMobility::Static);
            }
        }));
        Agent.ServerRPC_Interact(&WorldWeapon);
        world->RemoveOnActorSpawnedHandler(spawn);
        Record(TEXT("WEAPON.PICKUP.ATTACH_FAILURE"), TEXT("world pickup: presentation attachment failure leaves no dependent actor and permits retry"), unchanged());
    }

    void CheckWeapons(AAgentPlayerController& PC, UCreditComponent& Credits, ABaseAgent& Agent, UValorantGameInstance& Game)
    {
        auto& Shop = *PC.ShopComponent;
        const auto* vandal = Game.GetWeaponData(14);
        const auto* phantom = Game.GetWeaponData(13);
        const auto* ares = Game.GetWeaponData(18);
        if (!vandal || !phantom || !ares || !ares->WeaponClass)
        {
            Record(TEXT("SHOP.WEAPON.CATALOG"), TEXT("weapons: primary catalog available"), false);
            return;
        }

        Credits.AddCredits(5000 - Credits.GetCurrentCredit());
        const bool bBoughtVandal = CheckRequestOrder(PC, Credits, vandal->WeaponID);
        const auto beforeReplacement = CountWeaponActors(*Agent.GetWorld());
        const TWeakObjectPtr<ABaseWeapon> unused = Agent.GetMainWeapon();
        const bool bBoughtPhantom = Shop.PurchaseWeapon(phantom->WeaponID);
        auto* owned = Agent.GetMainWeapon();
        const bool bRefund = bBoughtVandal && bBoughtPhantom && !unused.IsValid()
            && owned && owned->GetWeaponID() == phantom->WeaponID && owned->GetOwnerAgent() == &Agent
            && Credits.GetCurrentCredit() == 5000 - phantom->Cost
            && CountWeaponActors(*Agent.GetWorld()) == beforeReplacement;
        Record(TEXT("SHOP.WEAPON.REFUND"), TEXT("weapons: new slot ownership, unused replacement refund, duplicate request"), bRefund
            && !Shop.PurchaseWeapon(phantom->WeaponID) && Credits.GetCurrentCredit() == 5000 - phantom->Cost);

        if (!owned)
        {
            return;
        }
        const int32 balance = Credits.GetCurrentCredit();
        UClass* aresClass = ares->WeaponClass.Get();
        bool bSpawnRejected = false;
        {
            TGuardValue<EClassFlags> abstractWeapon(aresClass->ClassFlags, aresClass->ClassFlags | CLASS_Abstract);
            bSpawnRejected = !Shop.PurchaseWeapon(ares->WeaponID);
        }
        const bool bSpawnPreserved = Agent.GetMainWeapon() == owned && Credits.GetCurrentCredit() == balance
            && CountWeaponActors(*Agent.GetWorld()) == beforeReplacement;
        Record(TEXT("SHOP.WEAPON.SPAWN_FAILURE"), TEXT("weapons: engine rejects a deferred spawn and preserves the old slot and balance"), bSpawnRejected && bSpawnPreserved);

        bool bSpawnedVisual = false;
        auto* world = Agent.GetWorld();
        const auto spawn = world->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
        {
            if (auto* visual = Cast<AThirdPersonInteractor>(Actor))
            {
                bSpawnedVisual = true;
                // A static component cannot be attached to the movable character mesh.
                visual->GetRootComponent()->SetMobility(EComponentMobility::Static);
            }
        }));
        const bool bAttachmentRejected = !Shop.PurchaseWeapon(ares->WeaponID);
        world->RemoveOnActorSpawnedHandler(spawn);
        Record(TEXT("SHOP.WEAPON.ATTACH_FAILURE"), TEXT("weapons: failed presentation attachment preserves the old slot and balance"),
            bSpawnedVisual && bAttachmentRejected && Agent.GetMainWeapon() == owned && Credits.GetCurrentCredit() == balance
            && CountWeaponActors(*world) == beforeReplacement);

        owned->SetWasUsed(true);
        const bool bDrop = Shop.PurchaseWeapon(ares->WeaponID)
            && IsValid(owned) && !owned->GetOwnerAgent() && Agent.GetMainWeapon()
            && Agent.GetMainWeapon()->GetWeaponID() == ares->WeaponID
            && Credits.GetCurrentCredit() == balance - ares->Cost;
        const auto afterDrop = CountWeaponActors(*world);
        if (bDrop)
        {
            CheckWorldPickup(Agent, *owned);
            Agent.ServerRPC_Interact(owned);
        }
        const auto afterPickup = CountWeaponActors(*world);
        UE_LOG(LogTemp, Display, TEXT("SHOP CHECK host weapon/visual counts: before=%d/%d drop=%d/%d pickup=%d/%d"),
            beforeReplacement.Key, beforeReplacement.Value, afterDrop.Key, afterDrop.Value, afterPickup.Key, afterPickup.Value);
        Record(TEXT("WEAPON.PICKUP.RETRY"), TEXT("weapons: used gun drops, can be picked up, and presentation count stays bounded"),
            bDrop && Agent.GetMainWeapon() == owned && owned->GetOwnerAgent() == &Agent
            && afterDrop.Key == beforeReplacement.Key + 1 && afterDrop.Value == beforeReplacement.Value
            && afterPickup == afterDrop && Credits.GetCurrentCredit() == balance - ares->Cost);
    }

    FAutoConsoleCommandWithWorldAndArgs ShopIntegrity(
        TEXT("Valorant.Shop.CheckIntegrity"),
        TEXT("Run balance and equipment assertions on the local host of a test Listen Server during Buy Phase."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* world)
        {
            auto* pc = IsUIRegressionEnvironment(world) && world->GetNetMode() == NM_ListenServer
                ? world->GetFirstPlayerController<AAgentPlayerController>() : nullptr;
            auto* agent = pc && pc->IsLocalController() ? pc->GetPawn<ABaseAgent>() : nullptr;
            auto* state = pc ? pc->GetPlayerState<AAgentPlayerState>() : nullptr;
            auto* game = world ? world->GetGameInstance<UValorantGameInstance>() : nullptr;
            if (!agent || !state || !game || !state->GetCreditComponent() || !state->GetBaseAttributeSet()
                || !pc->ShopComponent || !pc->ShopComponent->IsShopAvailable())
            {
                UE_LOG(LogTemp, Warning, TEXT("SHOP CHECK BLOCKED requires -UIRegressionReplay, loopback Listen Server, and a locked-in host in Buy Phase"));
                return;
            }

            CheckArmor(*pc->ShopComponent, *state->GetCreditComponent(), *state->GetBaseAttributeSet());
            CheckWeapons(*pc, *state->GetCreditComponent(), *agent, *game);
        }));
}

#endif
