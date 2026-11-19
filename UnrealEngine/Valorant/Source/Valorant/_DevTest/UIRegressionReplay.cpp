#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING
#include "CommonActivatableWidget.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/ValorantGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Player/Component/AgentInputComponent.h"
#include "Player/Component/CreditComponent.h"
#include "UI/MatchMap/MatchMapShopUI.h"
#include "UI/MatchMap/ShopViewModel.h"
#include "UI/MatchMap/MatchMapHUD.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "Weapon/BaseWeapon.h"
#include "Weapon/ThirdPersonInteractor.h"
#include "UIRegressionEnvironment.h"

namespace
{
    bool HasPresentedWidget(const AAgentPlayerController* PC, const TCHAR* ClassName)
    {
        for (TObjectIterator<UUserWidget> item; item; ++item)
        {
            if (item->GetOwningPlayer() == PC && item->GetClass()->GetName() == ClassName
                && (item->IsInViewport() || item->GetParent()))
            {
                return true;
            }
        }
        return false;
    }

    int32 CountPlayerCards(const AAgentPlayerController& PC)
    {
        const auto* hud = PC.GetMatchMapHud();
        const auto* top = hud ? Cast<UUserWidget>(hud->GetWidgetFromName(TEXT("TopWidget"))) : nullptr;
        const auto* blue = top ? Cast<UPanelWidget>(top->GetWidgetFromName(TEXT("StackBoxBlue"))) : nullptr;
        const auto* red = top ? Cast<UPanelWidget>(top->GetWidgetFromName(TEXT("StackBoxRed"))) : nullptr;
        return blue && red ? blue->GetChildrenCount() + red->GetChildrenCount() : INDEX_NONE;
    }

    void Key(const FKey& Key)
    {
        const FKeyEvent event(Key, FModifierKeysState(), 0, false, 0, 0);
        FSlateApplication::Get().ProcessKeyDownEvent(event);
        FSlateApplication::Get().ProcessKeyUpEvent(event);
    }

    void Result(const TCHAR* ID, const TCHAR* Name, bool bOk)
    {
        UE_LOG(LogTemp, Display, TEXT("REPLAY %s [%s] %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), ID, Name);
    }

    void CheckTimeout(AAgentPlayerController& PC, AAgentPlayerState& State)
    {
        static int32 stage = 0;
        static TStrongObjectPtr<UShopViewModel> viewModel;
        static FDelegateHandle responseHandle;
        static uint32 heldRequest = 0;
        static int32 originalCredit = 0;
        static double requestTime = 0;
        static TWeakObjectPtr<AAgentPlayerController> controller;
        if (stage == 0 && PC.ShopComponent->IsShopAvailable() && State.GetCurrentCredit() == 800)
        {
            controller = &PC;
            originalCredit = State.GetCurrentCredit();
            requestTime = PC.GetWorld()->GetTimeSeconds();
            viewModel.Reset(NewObject<UShopViewModel>(&PC));
            viewModel->Bind(PC);
            viewModel->BeginSession();
            viewModel->SelectItem(FShopItem{3, TEXT("Shorty"), EShopItemType::Weapon, 300});
            // Record the real response but keep it out of this auxiliary ViewModel until its timer fires.
            PC.OnShopPurchaseResponse.RemoveAll(viewModel.Get());
            responseHandle = PC.OnShopPurchaseResponse.AddLambda(
                [](uint32 Request, bool bSuccess, int32 ItemID, EShopItemType Type, const FString&)
                {
                    if (bSuccess && ItemID == 3 && Type == EShopItemType::Weapon)
                    {
                        heldRequest = Request;
                    }
                });
            const bool bRequested = viewModel->RequestPurchase();
            Result(TEXT("SHOP.REQUEST.PENDING_CLICK"), TEXT("timeout fixture sends one purchase and blocks a second click"), bRequested && !viewModel->RequestPurchase());
            stage = 1;
        }
        else if (stage == 1 && (controller.Get() != &PC || !viewModel->bPurchasePending
            || PC.GetWorld()->GetTimeSeconds() - requestTime > 15.0))
        {
            PC.OnShopPurchaseResponse.Remove(responseHandle);
            auto* agent = PC.GetPawn<ABaseAgent>();
            Result(TEXT("SHOP.REQUEST.TIMEOUT"), TEXT("timeout clears UI pending while real weapon and debit replicate"),
                heldRequest != 0 && !viewModel->bPurchasePending && !viewModel->PurchaseResultText.IsEmpty()
                && State.GetCurrentCredit() == originalCredit - 300 && agent && agent->GetSubWeapon()
                && agent->GetSubWeapon()->GetWeaponID() == 3);
            viewModel->Unbind();
            viewModel->Bind(PC);
            viewModel->BeginSession();
            viewModel->SelectItem(FShopItem{1, TEXT("Light Armor"), EShopItemType::Armor, 400});
            const bool bNewRequest = viewModel->RequestPurchase();
            PC.OnShopPurchaseResponse.Broadcast(heldRequest, true, 3, EShopItemType::Weapon, FString());
            Result(TEXT("SHOP.REQUEST.STALE_REPLY"), TEXT("held success cannot complete the new request"), bNewRequest
                && viewModel->bPurchasePending && viewModel->PurchaseResultText.IsEmpty());
            stage = bNewRequest ? 2 : 3;
        }
        else if (stage == 2 && !viewModel->bPurchasePending && State.GetArmor() == 25.f
            && State.GetCurrentCredit() == originalCredit - 700)
        {
            Result(TEXT("SHOP.REQUEST.AFTER_TIMEOUT"), TEXT("new response completes once and preserves both server purchases"),
                viewModel->TakePurchaseOutcome().IsSet() && !viewModel->TakePurchaseOutcome().IsSet());
            stage = 3;
        }
        if (stage == 3 && viewModel.IsValid())
        {
            viewModel->Unbind();
            viewModel.Reset();
            UE_LOG(LogTemp, Display, TEXT("REPLAY timeout fixture complete"));
        }
    }

    void CheckFireRelease(AAgentPlayerController& PC, ABaseAgent& Agent, const AAgentPlayerState& State)
    {
        static int32 stage = 0;
        static int32 ammoAtOpen = 0;
        static double deadline = 0;
        auto* weapon = Agent.GetSubWeapon();
        auto* input = Agent.FindComponentByClass<UAgentInputComponent>();
        if (State.GetArmor() != 25.f || State.GetCurrentCredit() != 100
            || !weapon || !input || !input->LeftMouseStartAction)
        {
            return;
        }
        auto* system = PC.GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        const double now = PC.GetWorld()->GetTimeSeconds();
        if (stage == 0 && PC.ShopComponent->IsShopAvailable())
        {
            Agent.SwitchEquipment(EInteractorType::SubWeapon);
            deadline = now + 2.0;
            stage = 1;
        }
        else if (stage == 1 && Agent.GetCurrentInterator() == weapon)
        {
            system->StartContinuousInputInjectionForAction(input->LeftMouseStartAction, true, {}, {});
            deadline = now + 2.0;
            stage = 2;
        }
        else if (stage == 1 && now >= deadline)
        {
            Result(TEXT("UI.INPUT.FIRE_RELEASE"), TEXT("opening during held fire stops the shot sequence"), false);
            stage = 4;
        }
        else if (stage == 2 && (weapon->MagazineAmmo < 2 || now >= deadline))
        {
            ammoAtOpen = weapon->MagazineAmmo;
            PC.RequestOpenShopUI();
            deadline = now + 0.65;
            stage = 3;
        }
        else if (stage == 3 && now >= deadline)
        {
            system->StopContinuousInputInjectionForAction(input->LeftMouseStartAction);
            Result(TEXT("UI.INPUT.FIRE_RELEASE"), TEXT("opening during held fire stops the shot sequence"),
                ammoAtOpen == 1 && weapon->MagazineAmmo == ammoAtOpen && PC.IsShopOpen());
            PC.RequestCloseShopUI();
            stage = 4;
        }
    }

    template <typename ActorType>
    int32 CountActors(UWorld& World)
    {
        int32 count = 0;
        for (TActorIterator<ActorType> actor(&World); actor; ++actor)
        {
            ++count;
        }
        return count;
    }

    FString Path(const TCHAR* Parameter, const TCHAR* Suffix)
    {
        FString file;
        FParse::Value(FCommandLine::Get(), Parameter, file);
        return file + Suffix;
    }

    void Tick(UWorld* world, ELevelTick, float)
    {
        static const int32 agentId = []
        {
            int32 value = 0;
            FParse::Value(FCommandLine::Get(), TEXT("UIRegressionAgent="), value);
            return value;
        }();
        if (agentId < 1 || agentId > 4 || !IsUIRegressionEnvironment(world))
        {
            return;
        }
        auto* pc = world->GetFirstPlayerController<AAgentPlayerController>();
        auto* state = pc ? pc->GetPlayerState<AAgentPlayerState>() : nullptr;
        auto* match = world->GetGameState<AMatchGameState>();
        if (!pc || !state || !match || !pc->IsLocalController())
        {
            return;
        }
        static bool bLocked = false;
        if (!bLocked && match->GetRoundSubState() == ERoundSubState::RSS_SelectAgent)
        {
            bLocked = true;
            pc->ServerRPC_LockIn(agentId);
            UE_LOG(LogTemp, Display, TEXT("REPLAY lock-in agent=%d"), agentId);
        }
        auto* pawn = pc->GetPawn<ABaseAgent>();
        if (!pawn || state->GetAgentID() != agentId || pawn->GetAgentID() != agentId)
        {
            return;
        }
        static bool bHostChecked = false;
        if (pc->HasAuthority())
        {
            if (!bHostChecked && match->CanOpenShop() && pc->ShopComponent->IsShopAvailable())
            {
                for (TActorIterator<AAgentPlayerState> other(world); other; ++other)
                {
                    if (other->GetAgentID() == 2 && other->GetCreditComponent())
                    {
                        other->GetCreditComponent()->AddCredits(5000 - other->GetCurrentCredit());
                        bHostChecked = true;
                        IConsoleManager::Get().ProcessUserConsoleInput(TEXT("Valorant.Shop.CheckIntegrity"), *GLog, world);
                        break;
                    }
                }
            }
            static double roundStart = 0;
            static int32 vitalsStage = 0;
            static TArray<TWeakObjectPtr<AThirdPersonInteractor>> victimVisuals;
            static TWeakObjectPtr<ABaseWeapon> droppedSidearm;
            if (bHostChecked && match->GetRoundSubState() == ERoundSubState::RSS_InRound)
            {
                if (roundStart == 0)
                {
                    roundStart = world->GetTimeSeconds();
                }
                for (TActorIterator<AAgentPlayerState> other(world); other; ++other)
                {
                    if (other->GetAgentID() != 2 || !other->GetBaseAttributeSet())
                    {
                        continue;
                    }
                    const double seconds = world->GetTimeSeconds() - roundStart;
                    if (vitalsStage == 0 && seconds > 13.0)
                    {
                        other->GetBaseAttributeSet()->SetHealth(24.f);
                        match->SetTeamScore(1, 0);
                        ++vitalsStage;
                    }
                    else if (vitalsStage == 1 && seconds > 17.0)
                    {
                        other->GetBaseAttributeSet()->SetHealth(100.f);
                        ++vitalsStage;
                    }
                    else if (vitalsStage >= 2 && vitalsStage <= 3 && seconds > (vitalsStage == 2 ? 21.0 : 25.0))
                    {
                        auto* victim = Cast<ABaseAgent>(other->GetPawn());
                        const auto* weapon = pawn->GetSubWeapon();
                        if (victim && weapon && weapon->NewDamageEffectClass)
                        {
                            if (vitalsStage == 3)
                            {
                                droppedSidearm = victim->GetSubWeapon();
                                for (TActorIterator<AThirdPersonInteractor> visual(world); visual; ++visual)
                                {
                                    if (visual->GetOwner() == victim)
                                    {
                                        victimVisuals.Add(*visual);
                                    }
                                }
                            }
                            victim->ServerApplyHitScanGE(weapon->NewDamageEffectClass,
                                vitalsStage == 2 ? 30 : 300, pawn, weapon->GetWeaponID());
                            ++vitalsStage;
                        }
                    }
                    else if (vitalsStage == 4 && seconds > 26.0)
                    {
                        Result(TEXT("WEAPON.LIFETIME.DEATH"), TEXT("death removes all captured presentation actors while sidearm stays in the world"),
                            !victimVisuals.IsEmpty()
                            && !victimVisuals.ContainsByPredicate([](const auto& visual) { return visual.IsValid(); })
                            && droppedSidearm.IsValid() && !droppedSidearm->GetOwnerAgent());
                        Result(TEXT("HUD.KILL_FEED"), TEXT("GAS kill creates a Blueprint kill-feed card on the host"),
                            other->GetHealth() == 0.f && HasPresentedWidget(pc, TEXT("WBP_KillFeed_C")));
                        ++vitalsStage;
                    }
                    break;
                }
            }
            return;
        }
        if (agentId == 3)
        {
            CheckTimeout(*pc, *state);
            CheckFireRelease(*pc, *pawn, *state);
        }
        if (agentId != 2)
        {
            return;
        }

        static int32 step = 0;
        static bool bModalShotRequested = false;
        static double next = 0;
        static FVector pos;
        static FRotator aim;
        static int32 baselineWeapons = 0;
        static int32 baselineVisuals = 0;
        static FVector closedPosition;
        static bool bStartedCrouchAndWalk = false;
        const double now = world->GetTimeSeconds();
        if (now < next)
        {
            return;
        }
        next = now + 0.65;

        UMatchMapShopUI* shop = nullptr;
        for (TObjectIterator<UMatchMapShopUI> item; item; ++item)
        {
            if (item->GetOwningPlayer() == pc && item->GetCachedWidget().IsValid())
            {
                shop = *item;
                break;
            }
        }
        UShopViewModel* vm = nullptr;
        for (TObjectIterator<UShopViewModel> item; item; ++item)
        {
            if (item->GetOuter() == pc)
            {
                vm = *item;
                break;
            }
        }
        const auto focused = [pc, shop](const TCHAR* Name)
        {
            auto* button = shop ? Cast<UButton>(shop->GetWidgetFromName(Name)) : nullptr;
            return button && button->HasUserFocus(pc);
        };
        const auto modal = [pc]()
        {
            for (TObjectIterator<UCommonActivatableWidget> item; item; ++item)
            {
                if (item->GetOwningPlayer() == pc && item->IsActivated() && item->GetClass()->GetName() == TEXT("ShopConfirmation"))
                {
                    return true;
                }
            }
            return false;
        };
        const auto inject = [pc, pawn](bool bSwitch)
        {
            auto* input = pawn->FindComponentByClass<UAgentInputComponent>();
            auto* system = pc->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
            if (!input || !system)
            {
                return false;
            }
            system->InjectInputForAction(input->MoveAction, FVector2D(0, 1), {}, {});
            system->InjectInputForAction(input->LookAction, FVector2D(15, 0), {}, {});
            system->InjectInputForAction(input->LeftMouseStartAction, true, {}, {});
            if (bSwitch)
            {
                system->InjectInputForAction(input->Num_3Action, true, {}, {});
                system->InjectInputForAction(input->CAction, true, {}, {});
            }
            else
            {
                system->StartContinuousInputInjectionForAction(input->MoveAction, FVector2D(0, 1), {}, {});
            }
            return true;
        };

        switch (step)
        {
        case 0:
            if (match->CanOpenShop() && state->GetCurrentCredit() == 5000)
            {
                if (auto* input = pawn->FindComponentByClass<UAgentInputComponent>())
                {
                    auto* system = pc->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
                    system->StartContinuousInputInjectionForAction(input->CtrlAction, true, {}, {});
                    system->StartContinuousInputInjectionForAction(input->ShiftAction, true, {}, {});
                    system->StartContinuousInputInjectionForAction(input->MoveAction, FVector2D(0, 1), {}, {});
                }
                step = -1;
            }
            break;
        case -1:
            {
                const auto* running = FindFProperty<FBoolProperty>(ABaseAgent::StaticClass(), TEXT("bIsRun"));
                bStartedCrouchAndWalk = pawn->bIsCrouched && running && !running->GetPropertyValue_InContainer(pawn);
                pc->RequestOpenShopUI();
                if (auto* input = pawn->FindComponentByClass<UAgentInputComponent>())
                {
                    auto* system = pc->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
                    system->StopContinuousInputInjectionForAction(input->MoveAction);
                    system->StopContinuousInputInjectionForAction(input->CtrlAction);
                    system->StopContinuousInputInjectionForAction(input->ShiftAction);
                }
                next = now + 1.0;
                step = 1;
            }
            break;
        case 1:
            if (!pc->IsShopOpen())
            {
                pc->RequestOpenShopUI();
                break;
            }
            Result(TEXT("UI.FOCUS.INITIAL"), TEXT("initial Classic focus"), focused(TEXT("ButtonClassic")));
            Result(TEXT("HUD.CARDS.INITIAL"), TEXT("buy phase has four top HUD cards"), CountPlayerCards(*pc) == 4);
            {
                const auto* running = FindFProperty<FBoolProperty>(ABaseAgent::StaticClass(), TEXT("bIsRun"));
                Result(TEXT("UI.INPUT.CROUCH_WALK_RELEASE"), TEXT("opening while crouched and walking releases both states"),
                    bStartedCrouchAndWalk && !pawn->bIsCrouched && running && running->GetPropertyValue_InContainer(pawn));
            }
            baselineWeapons = CountActors<ABaseWeapon>(*world);
            baselineVisuals = CountActors<AThirdPersonInteractor>(*world);
            FScreenshotRequest::RequestScreenshot(Path(TEXT("UIRegressionShop="), TEXT("")), true, false);
            Key(EKeys::Gamepad_LeftShoulder);
            ++step;
            break;
        case 2:
            Result(TEXT("UI.FOCUS.CATEGORY"), TEXT("armor category focuses Light Shield"), focused(TEXT("ButtonLightShiled")));
            Key(EKeys::Gamepad_DPad_Down);
            ++step;
            break;
        case 3:
            Result(TEXT("UI.FOCUS.DPAD"), TEXT("D-pad skips Regen and focuses Heavy Shield"), focused(TEXT("ButtonHeavyShiled")));
            Key(EKeys::Gamepad_FaceButton_Bottom);
            ++step;
            break;
        case 4:
            if (!bModalShotRequested)
            {
                Result(TEXT("UI.MODAL.ARMOR"), TEXT("Heavy Shield maps to item 2 and opens modal"), modal() && vm && vm->SelectedItemId == 2);
                FScreenshotRequest::RequestScreenshot(Path(TEXT("UIRegressionShop="), TEXT("-confirm.png")), true, false);
                bModalShotRequested = true;
                break;
            }
            Key(EKeys::Gamepad_FaceButton_Right);
            ++step;
            break;
        case 5:
            Result(TEXT("UI.MODAL.CANCEL"), TEXT("cancel restores the Heavy Shield button"), !modal() && focused(TEXT("ButtonHeavyShiled")));
            Key(EKeys::Gamepad_FaceButton_Bottom);
            ++step;
            break;
        case 6:
            Key(EKeys::Gamepad_FaceButton_Bottom);
            Result(TEXT("SHOP.REQUEST.PENDING"), TEXT("confirm emits a pending remote purchase"), vm && vm->bPurchasePending);
            pc->CloseShopUI();
            pc->RequestOpenShopUI();
            pc->OnShopPurchaseResponse.Broadcast(0, true, 2, EShopItemType::Armor, TEXT("legacy"));
            Result(TEXT("SHOP.SESSION.REOPEN_PENDING"), TEXT("screen reopen preserves pending and ignores uncorrelated legacy reply"),
                vm && vm->bPurchasePending && vm->SelectedItemId == 0 && vm->PurchaseResultText.IsEmpty());
            ++step;
            next = now + 1.2;
            break;
        case 7:
            {
                const auto* text = shop ? Cast<UTextBlock>(shop->GetWidgetFromName(TEXT("CreditText"))) : nullptr;
                Result(TEXT("SHOP.SESSION.LATE_REPLY"), TEXT("late Heavy Shield reply updates replication, model and UI without restoring old feedback"),
                    state->GetArmor() == 50.f && state->GetShopArmorTier() == 2 && state->GetCurrentCredit() == 4000
                    && vm && vm->CurrentCredits == 4000 && !vm->bPurchasePending && vm->PurchaseResultText.IsEmpty()
                    && text && text->GetText().ToString() == TEXT("4,000"));
                pc->OnShopPurchaseResponse.Broadcast(1, true, 2, EShopItemType::Armor, TEXT("duplicate"));
                Result(TEXT("SHOP.SESSION.DUPLICATE_REPLY"), TEXT("duplicate response leaves the reopened shop unchanged"),
                    vm && !vm->bPurchasePending && vm->PurchaseResultText.IsEmpty() && vm->SelectedItemId == 0);
                // The server independently rejects a duplicate even when bypassing UI validation.
                pc->Server_RequestShopPurchase(2, EShopItemType::Armor, pc->AllocateShopRequestID());
                Key(EKeys::Gamepad_LeftShoulder);
                ++step;
            }
            break;
        case 8:
            Key(EKeys::Gamepad_DPad_Down);
            ++step;
            break;
        case 9:
            Key(EKeys::Gamepad_FaceButton_Bottom);
            Result(TEXT("SHOP.ARMOR.CLIENT_DUPLICATE"), TEXT("full Heavy Shield cannot open a second modal"), !modal() && vm && vm->SelectedItemId == 2
                && !vm->bCanPurchase && state->GetCurrentCredit() == 4000);
            Key(EKeys::Gamepad_RightShoulder);
            ++step;
            break;
        case 10:
            Key(EKeys::Gamepad_DPad_Down);
            ++step;
            break;
        case 11:
            Result(TEXT("UI.FOCUS.WEAPON"), TEXT("weapon navigation focuses Shorty"), focused(TEXT("ButtonShorty")));
            Key(EKeys::Gamepad_FaceButton_Bottom);
            ++step;
            break;
        case 12:
            Result(TEXT("UI.MODAL.WEAPON"), TEXT("Shorty opens weapon confirmation"), modal() && vm && vm->SelectedItemId == 3);
            Key(EKeys::Gamepad_FaceButton_Bottom);
            ++step;
            next = now + 1.0;
            break;
        case 13:
            {
                const auto* weapon = pawn->GetSubWeapon();
                Result(TEXT("SHOP.WEAPON.REPLICATION"), TEXT("weapon purchase replicates slot and balance to the remote owner"),
                    weapon && weapon->GetWeaponID() == 3 && weapon->GetOwnerAgent() == pawn && state->GetCurrentCredit() == 3700);
                const int32 weapons = CountActors<ABaseWeapon>(*world);
                const int32 visuals = CountActors<AThirdPersonInteractor>(*world);
                auto* game = world->GetGameInstance<UValorantGameInstance>();
                const auto* row = game ? game->GetWeaponData(3) : nullptr;
                bool bRemoteMesh = false;
                for (TActorIterator<AThirdPersonInteractor> visual(world); visual; ++visual)
                {
                    bRemoteMesh |= visual->GetOwner() == pawn && row && visual->Mesh->GetSkeletalMeshAsset() == row->WeaponMesh;
                }
                UE_LOG(LogTemp, Display, TEXT("REPLAY remote weapon/visual counts: before=%d/%d after=%d/%d"),
                    baselineWeapons, baselineVisuals, weapons, visuals);
                Result(TEXT("WEAPON.LIFETIME.REPLACEMENT"), TEXT("remote unused replacement keeps counts and initializes the replicated mesh"),
                    bRemoteMesh && weapons == baselineWeapons && visuals == baselineVisuals);
                pawn->SwitchEquipment(EInteractorType::SubWeapon);
                Key(EKeys::Gamepad_RightShoulder);
                step = 131;
            }
            break;
        case 131:
            Result(TEXT("UI.FOCUS.SKILL"), TEXT("skill category focuses the authored Phoenix C button"), focused(TEXT("ButtonSkillC")));
            Key(EKeys::Gamepad_FaceButton_Bottom);
            step = 132;
            break;
        case 132:
            Result(TEXT("UI.MODAL.SKILL"), TEXT("Phoenix C authored button sends the current agent skill ID"), modal() && vm && vm->SelectedItemId == 2001);
            Key(EKeys::Gamepad_FaceButton_Bottom);
            step = 14;
            break;
        case 14:
            {
                const auto* hud = Cast<UMatchMapHUD>(pc->GetMatchMapHud());
                Result(TEXT("SHOP.SKILL.REPLICATION"), TEXT("Phoenix C purchase replicates into HUD and debits 300"),
                    state->GetAbilityStack(2001) == 1 && state->GetCurrentCredit() == 3400
                    && hud && hud->GetAbilityInfoBySlot(EAbilitySlotType::Slot_C).CurrentStack == 1);
                pc->Server_RequestShopPurchase(2001, EShopItemType::Ability, pc->AllocateShopRequestID());
            }
            pos = pawn->GetActorLocation();
            aim = pc->GetControlRotation();
            Result(TEXT("UI.INPUT.INJECT"), TEXT("Enhanced Input action injection started while shop is open"), inject(true) && pc->IsShopOpen());
            ++step;
            break;
        case 15:
            {
                const auto* weapon = pawn->GetSubWeapon();
                Result(TEXT("UI.INPUT.BLOCK"), TEXT("open shop blocks movement, aim, fire, skill and weapon switch"),
                    FVector::Dist2D(pos, pawn->GetActorLocation()) < 4.0 && pc->GetControlRotation().Equals(aim, 0.05)
                    && weapon && weapon->MagazineAmmo == 2 && pawn->GetCurrentInterator() == weapon
                    && state->GetAbilityStack(2001) == 1 && state->GetCurrentCredit() == 3400);
                pc->InputKey(FInputKeyParams(EKeys::LeftMouseButton, IE_Pressed, 1.0, false));
                pc->InputKey(FInputKeyParams(EKeys::LeftMouseButton, IE_Released, 0.0, false));
                pc->InputKey(FInputKeyParams(EKeys::LeftMouseButton, IE_Pressed, 1.0, false));
                pc->InputKey(FInputKeyParams(EKeys::W, IE_Pressed, 1.0, false));
                closedPosition = pawn->GetActorLocation();
                ++step;
            }
            break;
        case 16:
            if (match->GetRoundSubState() == ERoundSubState::RSS_InRound)
            {
                Result(TEXT("UI.INPUT.AUTO_CLOSE"), TEXT("round start clears shop stack"), !pc->IsShopOpen());
                Result(TEXT("HUD.CARDS.ROUND"), TEXT("round transition preserves exactly four top HUD cards"), CountPlayerCards(*pc) == 4);
                pc->Server_RequestShopPurchase(1, EShopItemType::Armor, pc->AllocateShopRequestID());
                FScreenshotRequest::RequestScreenshot(Path(TEXT("UIRegressionHud="), TEXT("")), true, false);
                step = 160;
            }
            break;
        case 160:
            {
                const auto* weapon = pawn->GetSubWeapon();
                Result(TEXT("UI.INPUT.HELD_KEYS"), TEXT("held shop keys do not carry movement or fire into gameplay"),
                    pc->PlayerInput && !pc->PlayerInput->IsPressed(EKeys::LeftMouseButton)
                    && !pc->PlayerInput->IsPressed(EKeys::W) && weapon && weapon->MagazineAmmo == 2
                    && FVector::Dist2D(closedPosition, pawn->GetActorLocation()) < 4.0);
                pc->InputKey(FInputKeyParams(EKeys::LeftMouseButton, IE_Released, 0.0, false));
                pc->InputKey(FInputKeyParams(EKeys::W, IE_Released, 0.0, false));
                pos = pawn->GetActorLocation();
                aim = pc->GetControlRotation();
                inject(false);
                step = 17;
            }
            break;
        case 17:
            {
                const auto* weapon = pawn->GetSubWeapon();
                Result(TEXT("UI.INPUT.RESTORE"), TEXT("gameplay input restored after closing"),
                    FVector::Dist2D(pos, pawn->GetActorLocation()) > 4.0 && !pc->GetControlRotation().Equals(aim, 0.05)
                    && weapon && weapon->MagazineAmmo < 2);
                if (auto* input = pawn->FindComponentByClass<UAgentInputComponent>())
                {
                    auto* system = pc->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
                    system->StopContinuousInputInjectionForAction(input->MoveAction);
                    system->InjectInputForAction(input->LeftMouseEndAction, true, {}, {});
                    system->InjectInputForAction(input->ReloadAction, true, {}, {});
                }
                next = now + 2.4;
                ++step;
            }
            break;
        case 18:
            {
                const auto* weapon = pawn->GetSubWeapon();
                Result(TEXT("HUD.REPLICATION.AMMO"), TEXT("remote release and reload propagate ammo"),
                    weapon && weapon->MagazineAmmo == 2 && weapon->SpareAmmo < 10 && state->GetCurrentCredit() == 3400);
                if (auto* input = pawn->FindComponentByClass<UAgentInputComponent>())
                {
                    pc->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->InjectInputForAction(input->CAction, true, {}, {});
                }
                next = now + 1.6;
                ++step;
            }
            break;
        case 19:
            {
                const auto* hud = Cast<UMatchMapHUD>(pc->GetMatchMapHud());
                const auto* hint = hud ? hud->GetWidgetFromName(TEXT("LeftOrRight_C")) : nullptr;
                Result(TEXT("HUD.SKILL.FOLLOW_UP"), TEXT("Phoenix preparation shows existing follow-up hint"), hint && hint->IsVisible());
                FScreenshotRequest::RequestScreenshot(Path(TEXT("UIRegressionHud="), TEXT("-hint.png")), true, false);
                if (auto* input = pawn->FindComponentByClass<UAgentInputComponent>())
                {
                    pc->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->InjectInputForAction(input->LeftMouseStartAction, true, {}, {});
                }
                next = now + 2.0;
                ++step;
            }
            break;
        case 20:
            {
                const auto* hud = Cast<UMatchMapHUD>(pc->GetMatchMapHud());
                Result(TEXT("HUD.SKILL.CONSUME"), TEXT("Phoenix confirmed ability consumes stack and refreshes HUD"),
                    state->GetAbilityStack(2001) == 0 && hud && hud->GetAbilityInfoBySlot(EAbilitySlotType::Slot_C).CurrentStack == 0);
                ++step;
            }
            break;
        case 21:
            // AttributeSet and GameState can reach the client in different packets.
            if (state->GetHealth() == 24.f && match->GetTeamBlueScore() == 1)
            {
                const auto* hud = pc->GetMatchMapHud();
                const auto* hp = hud ? Cast<UTextBlock>(hud->GetWidgetFromName(TEXT("txt_HP"))) : nullptr;
                const auto* top = hud ? Cast<UUserWidget>(hud->GetWidgetFromName(TEXT("TopWidget"))) : nullptr;
                const auto* score = top ? Cast<UTextBlock>(top->GetWidgetFromName(TEXT("TextBlockBlueScore"))) : nullptr;
                Result(TEXT("HUD.REPLICATION.VITALS_SCORE"), TEXT("server health and score change reach remote HUD"),
                    hp && hp->GetText().ToString() == TEXT("24") && score && score->GetText().ToString() == TEXT("1"));
                FScreenshotRequest::RequestScreenshot(Path(TEXT("UIRegressionHud="), TEXT("-vitals.png")), true, false);
                ++step;
            }
            break;
        case 22:
            if (state->GetHealth() == 100.f)
            {
                pc->ClientRPC_DisplayHud(false);
                pc->ClientRPC_DisplayHud(true);
                pc->ClientRPC_DisplayHud(false);
                pc->ClientRPC_DisplayHud(true);
                const auto* hud = pc->GetMatchMapHud();
                const auto* hp = hud ? Cast<UTextBlock>(hud->GetWidgetFromName(TEXT("txt_HP"))) : nullptr;
                const auto* armor = hud ? Cast<UTextBlock>(hud->GetWidgetFromName(TEXT("txt_Armor"))) : nullptr;
                const auto* keyLabel = hud ? Cast<UTextBlock>(hud->GetWidgetFromName(TEXT("TextBlock_101"))) : nullptr;
                Result(TEXT("HUD.RECREATE"), TEXT("two HUD recreations restore vitals and keep four cards"),
                    hp && armor && hp->GetText().ToString() == TEXT("100")
                    && armor->GetText().ToString() == TEXT("50") && CountPlayerCards(*pc) == 4
                    && keyLabel && keyLabel->GetText().ToString() == TEXT("C  0/1"));
                ++step;
            }
            break;
        case 23:
            if (state->GetHealth() < 100.f && state->GetHealth() > 0.f)
            {
                Result(TEXT("HUD.DAMAGE_INDICATOR"), TEXT("GAS damage creates a Blueprint damage indicator"),
                    HasPresentedWidget(pc, TEXT("WBP_MatchMap_Indicator_C")));
                FScreenshotRequest::RequestScreenshot(Path(TEXT("UIRegressionHud="), TEXT("-damage.png")), true, false);
                UE_LOG(LogTemp, Display, TEXT("REPLAY completed agent=%d armor=%.0f credit=%d"), state->GetAgentID(), state->GetArmor(), state->GetCurrentCredit());
                ++step;
            }
            break;
        default:
            next = now + 60;
            break;
        }
    }

    struct FReplayRegistration
    {
        FReplayRegistration() { FWorldDelegates::OnWorldPostActorTick.AddStatic(&Tick); }
    } Registration;
}
#endif
