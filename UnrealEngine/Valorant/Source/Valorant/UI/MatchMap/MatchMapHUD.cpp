// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHUD.h"

#include "MatchMapHudTopWidget.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "Valorant.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/Agent/BaseAgent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMatchHUD, Log, All);

#define LOCTEXT_NAMESPACE "MatchMapHUD"

void UMatchMapHUD::SetTrueVo()
{
	bPlayed60SecLeftVo = true;
	bPlayed30SecLeftVo = true;
	bPlayed10SecLeftVo = true;
}

void UMatchMapHUD::SetFalseVo()
{
	bPlayed60SecLeftVo = false;
	bPlayed30SecLeftVo = false;
	bPlayed10SecLeftVo = false;
}

void UMatchMapHUD::NativeConstruct()
{
    Super::NativeConstruct();
    bHudConstructed = true;

    if (!CombatViewModel)
    {
        CombatViewModel = NewObject<UCombatHUDViewModel>(this);
        AbilityBar = NewObject<UHUDAbilityBarModel>(this);
    }
    CacheAbilityWidgets();

    AbilityBar->OnSlotChanged.RemoveAll(this);
    AbilityBar->OnSlotChanged.AddUObject(this, &UMatchMapHUD::OnAbilitySlotChanged);

    GetWorld()->GameStateSetEvent.RemoveAll(this);
    GetWorld()->GameStateSetEvent.AddUObject(this, &UMatchMapHUD::BindMatchState);
    BindMatchState(GetWorld()->GetGameState());
    RefreshPlayerSources();

    CombatViewModel->RemoveAllFieldValueChangedDelegates(this);
    CombatViewModel->GetFieldNotificationDescriptor().ForEachField(
        CombatViewModel->GetClass(), [this](UE::FieldNotification::FFieldId field)
        {
            CombatViewModel->AddFieldValueChangedDelegate(
                field, UCombatHUDViewModel::FFieldValueChangedDelegate::CreateUObject(this, &UMatchMapHUD::OnStatusFieldChanged));
            return true;
        });

    RenderVitals();
    RenderAmmo();
    UpdateCreditDisplay(CombatViewModel->Credits);
}

void UMatchMapHUD::NativeDestruct()
{
    bHudConstructed = false;
    if (auto* world = GetWorld())
    {
        world->GameStateSetEvent.RemoveAll(this);
        world->GetTimerManager().ClearTimer(AnnouncementTimerHandle);
        CancelLatentActions();
    }
    HideAnnouncement();
    HideFollowUpInputUI();
    bIsPreRound = false;
    SetTrueVo();
    BindMatchState(nullptr);
    UnbindPlayer();
    RestoreAbilityWidgets();
    Super::NativeDestruct();
}

void UMatchMapHUD::BindMatchState(AGameStateBase* State)
{
    auto* match = Cast<AMatchGameState>(State);
    if (BoundMatchState.Get() == match && !BoundMatchState.IsStale())
    {
        return;
    }
    if (auto* previous = BoundMatchState.Get())
    {
        previous->OnRemainRoundStateTimeChanged.RemoveDynamic(this, &UMatchMapHUD::UpdateTime);
        previous->OnRoundSubStateChanged.RemoveDynamic(this, &UMatchMapHUD::OnRoundSubStateChanged);
        previous->OnRoundEnd.RemoveDynamic(this, &UMatchMapHUD::OnRoundEnd);
    }

    BoundMatchState = match;
    TopWidget->BindMatchState(match);
    if (match)
    {
        match->OnRemainRoundStateTimeChanged.AddUniqueDynamic(this, &UMatchMapHUD::UpdateTime);
        match->OnRoundSubStateChanged.AddUniqueDynamic(this, &UMatchMapHUD::OnRoundSubStateChanged);
        match->OnRoundEnd.AddUniqueDynamic(this, &UMatchMapHUD::OnRoundEnd);
    }
}

void UMatchMapHUD::RefreshPlayerSources()
{
    auto* controller = GetOwningPlayer<AAgentPlayerController>();
    if (!bHudConstructed || !controller)
    {
        return;
    }

    if (BoundController.Get() != controller)
    {
        if (auto* previous = BoundController.Get())
        {
            previous->OnDamaged_PC.RemoveDynamic(this, &UMatchMapHUD::OnDamaged);
            previous->OnHealed_PC.RemoveDynamic(this, &UMatchMapHUD::OnHealed);
            previous->OnKillEvent_PC.RemoveDynamic(this, &UMatchMapHUD::OnKillEvent);
            previous->OnPossessedPawnChanged.RemoveDynamic(this, &UMatchMapHUD::OnPawnChanged);
        }
        BoundController = controller;
        controller->OnDamaged_PC.AddUniqueDynamic(this, &UMatchMapHUD::OnDamaged);
        controller->OnHealed_PC.AddUniqueDynamic(this, &UMatchMapHUD::OnHealed);
        controller->OnKillEvent_PC.AddUniqueDynamic(this, &UMatchMapHUD::OnKillEvent);
        controller->OnPossessedPawnChanged.AddUniqueDynamic(this, &UMatchMapHUD::OnPawnChanged);
    }

    const auto suppressionTag = FValorantGameplayTags::Get().State_Debuff_Suppressed;
    auto* state = controller->GetPlayerState<AAgentPlayerState>();
    auto* asc = state ? state->GetAbilitySystemComponent() : nullptr;
    if (BoundASC.Get() != asc || BoundASC.IsStale())
    {
        if (auto* previous = BoundASC.Get())
        {
            previous->RegisterGameplayTagEvent(suppressionTag, EGameplayTagEventType::NewOrRemoved).Remove(SuppressionHandle);
        }
        BoundASC = asc;
        SuppressionHandle.Reset();
        if (asc)
        {
            SuppressionHandle = asc->RegisterGameplayTagEvent(suppressionTag, EGameplayTagEventType::NewOrRemoved)
                .AddUObject(this, &UMatchMapHUD::OnSuppressed);
        }
        OnSuppressed(suppressionTag, asc ? asc->GetTagCount(suppressionTag) : 0);
    }

    const bool bPlayerStateChanged = BoundPlayerState.Get() != state || BoundPlayerState.IsStale();
    if (bPlayerStateChanged)
    {
        if (auto* previous = BoundPlayerState.Get())
        {
            previous->OnAgentIdChanged.RemoveAll(this);
        }
        BoundPlayerState = state;
        if (state)
        {
            state->OnAgentIdChanged.AddUObject(this, &UMatchMapHUD::RefreshPlayerSources);
        }
    }

    CombatViewModel->Bind(*controller);
    if (AbilityBar->Bind(state, GetWorld()->GetGameInstance<UValorantGameInstance>()))
    {
        for (const EAbilitySlotType slot : UHUDAbilityBarModel::SlotTypes)
        {
            RenderAbilitySlot(slot, true);
        }
    }
    if (bPlayerStateChanged && state && !state->HasAuthority())
    {
        state->Server_RequestAbilityStackSync();
    }
}

void UMatchMapHUD::UnbindPlayer()
{
    if (auto* controller = BoundController.Get())
    {
        controller->OnDamaged_PC.RemoveDynamic(this, &UMatchMapHUD::OnDamaged);
        controller->OnHealed_PC.RemoveDynamic(this, &UMatchMapHUD::OnHealed);
        controller->OnKillEvent_PC.RemoveDynamic(this, &UMatchMapHUD::OnKillEvent);
        controller->OnPossessedPawnChanged.RemoveDynamic(this, &UMatchMapHUD::OnPawnChanged);
    }
    if (auto* state = BoundPlayerState.Get())
    {
        state->OnAgentIdChanged.RemoveAll(this);
    }
    if (auto* asc = BoundASC.Get())
    {
        asc->RegisterGameplayTagEvent(FValorantGameplayTags::Get().State_Debuff_Suppressed, EGameplayTagEventType::NewOrRemoved)
            .Remove(SuppressionHandle);
    }

    CombatViewModel->RemoveAllFieldValueChangedDelegates(this);
    CombatViewModel->Unbind();
    AbilityBar->OnSlotChanged.RemoveAll(this);
    AbilityBar->Unbind();
    BoundController.Reset();
    BoundPlayerState.Reset();
    BoundASC.Reset();
    SuppressionHandle.Reset();
}

void UMatchMapHUD::OnPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    HideFollowUpInputUI();
    RefreshPlayerSources();
}

void UMatchMapHUD::UpdateTime(float Time)
{
	if (bIsPreRound && FMath::IsNearlyEqual(Time, 3.f, 0.5f))
	{
		PlayRemTimeVO(3);
		bIsPreRound = false;
	}
	if (Time <= 60)
	{
		if (false == bPlayed60SecLeftVo && FMath::IsNearlyEqual(Time, 60.f, 0.5f))
		{
			PlayRemTimeVO(0);
			bPlayed60SecLeftVo = true;
		}
		if (Time <= 30)
		{
			if (false == bPlayed30SecLeftVo && FMath::IsNearlyEqual(Time, 30.f, 0.5f))
			{
				PlayRemTimeVO(1);
				bPlayed30SecLeftVo = true;
			}
			if (Time <= 10)
			{
				if (false == bPlayed10SecLeftVo && FMath::IsNearlyEqual(Time, 10.f, 0.5f))
				{
					PlayRemTimeVO(2);
					bPlayed10SecLeftVo = true;
				}
			}
		}
	}
}

void UMatchMapHUD::OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime)
{
	SetTrueVo();
	switch (RoundSubState) {
	case ERoundSubState::RSS_None:
		break;
	case ERoundSubState::RSS_SelectAgent:
		break;
	case ERoundSubState::RSS_PreRound:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, TransitionTime);
		break;
	case ERoundSubState::RSS_BuyPhase:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, TransitionTime);
		break;
	case ERoundSubState::RSS_InRound:
		HideAnnouncement();
		GetWorld()->GetTimerManager().ClearTimer(AnnouncementTimerHandle);
		bIsPreRound = false;
		SetFalseVo();
		break;
	case ERoundSubState::RSS_EndPhase:
		break;
	}
	DebugRoundSubState(StaticEnum<ERoundSubState>()->GetNameStringByValue(static_cast<int>(RoundSubState)));
}

void UMatchMapHUD::OnRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason, const float TransitionTime)
{
	auto* PlayerState = GetOwningPlayer()->GetPlayerState<AMatchPlayerState>();
	if (nullptr == PlayerState)
	{
		return;
	}

	bool bIsPlayerWin = (PlayerState->bIsBlueTeam == bBlueWin);
	
	// 라운드 종료 이유에 따라 적절한 UI 표시
	switch (RoundEndReason)
	{
	case ERoundEndReason::ERER_SpikeActive:
		// 스파이크 폭발로 인한 종료
		if (bIsPlayerWin)
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeActivated_Won, TransitionTime);
		}
		else
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeActivated_Lost, TransitionTime);
		}
		break;
	case ERoundEndReason::ERER_SpikeDefuse:
		// 스파이크 해제로 인한 종료
		if (bIsPlayerWin)
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeDefused_Won, TransitionTime);
		}
		else
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeDefused_Lost, TransitionTime);
		}
		break;
	default:
		// 일반적인 승리/패배
		if (bIsPlayerWin)
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_Won, TransitionTime);
		}
		else
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_Lost, TransitionTime);
		}
		break;
	}
	
	PlayRoundEndVFX(bIsPlayerWin);
}

void UMatchMapHUD::DisplayAnnouncement(EMatchAnnouncement MatchAnnouncement, float DisplayTime)
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, Idx: %d, TransitionTime: %f"), __FUNCTION__, static_cast<int32>(MatchAnnouncement), DisplayTime);
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Visible);
	WidgetSwitcherAnnouncement->SetActiveWidgetIndex(static_cast<int32>(MatchAnnouncement));

	TopWidget->CanvasPanelTimer->SetVisibility(ESlateVisibility::Visible);
	TopWidget->ImageSpike->SetVisibility(ESlateVisibility::Hidden);
	
	OnLowState(false);
	HideFollowUpInputUI();
	OnSpikeOwnChanged(false);
	
	GetWorld()->GetTimerManager().SetTimer(AnnouncementTimerHandle, this, &UMatchMapHUD::HideAnnouncement, DisplayTime, false);
	bIsPreRound = MatchAnnouncement == EMatchAnnouncement::EMA_BuyPhase;
}

void UMatchMapHUD::HideAnnouncement()
{
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Hidden);
}

void UMatchMapHUD::OnStatusFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field)
{
    if (Field.GetName() == GET_MEMBER_NAME_CHECKED(UCombatHUDViewModel, Vitals))
    {
        RenderVitals();
    }
    else if (Field.GetName() == GET_MEMBER_NAME_CHECKED(UCombatHUDViewModel, Ammo))
    {
        RenderAmmo();
    }
    else if (Field.GetName() == GET_MEMBER_NAME_CHECKED(UCombatHUDViewModel, Credits))
    {
        UpdateCreditDisplay(CombatViewModel->Credits);
        UE_LOG(LogMatchHUD, Verbose, TEXT("HUD credits=%d"), CombatViewModel->Credits);
    }
}

void UMatchMapHUD::RenderVitals()
{
    const auto& vitals = CombatViewModel->Vitals;
    const bool bKnown = vitals.MaxHealth > 0;
    const bool bLow = bKnown && vitals.Health <= vitals.MaxHealth * LowHealthFraction;
    txt_HP->SetText(bKnown ? FText::AsNumber(vitals.Health) : LOCTEXT("UnknownHealth", "—"));
    txt_HP->SetColorAndOpacity(bLow ? WarningTextColor : FLinearColor::White);
    txt_Armor->SetText(bKnown ? FText::AsNumber(vitals.Armor) : LOCTEXT("UnknownArmor", "—"));
    txt_Armor->SetColorAndOpacity(vitals.Armor > 0 ? SecondaryTextColor : UnavailableSkillColor);
    UE_LOG(LogMatchHUD, Verbose, TEXT("HUD vitals=%d/%d armor=%d"), vitals.Health, vitals.MaxHealth, vitals.Armor);
}

void UMatchMapHUD::RenderAmmo()
{
    const auto& ammo = CombatViewModel->Ammo;
    HorizontalBoxAmmo->SetVisibility(ammo.bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    if (ammo.bVisible)
    {
        TextBlockMagazineAmmo->SetText(FText::AsNumber(ammo.Magazine));
        TextBlockMagazineAmmo->SetColorAndOpacity(ammo.Magazine == 0 ? WarningTextColor : FLinearColor::White);
        TextBlockSpareAmmo->SetText(FText::Format(LOCTEXT("ReserveAmmo", "/ {0}"), FText::AsNumber(ammo.Reserve)));
        TextBlockSpareAmmo->SetColorAndOpacity(SecondaryTextColor);
    }
    UE_LOG(LogMatchHUD, Verbose, TEXT("HUD ammo=%d/%d visible=%d"), ammo.Magazine, ammo.Reserve, ammo.bVisible);
}

void UMatchMapHUD::OnDamaged(const FVector& HitOrg, const EAgentDamagedPart DamagedPart,
	const EAgentDamagedDirection DamagedDirection, const bool bDie, const bool bLarge, const bool bLowState)
{
	OnDisplayIndicator(HitOrg);
	if (bLowState)
	{
		OnLowState(true);
	}
}

void UMatchMapHUD::BindToDelegatePC(UAgentAbilitySystemComponent* asc, AAgentPlayerController* pc)
{
    if (pc && pc == GetOwningPlayer())
    {
        RefreshPlayerSources();
    }
}

void UMatchMapHUD::InitUI(AAgentPlayerState* ps)
{
    if (ps && GetOwningPlayer() && ps == GetOwningPlayer()->GetPlayerState<AAgentPlayerState>())
    {
        RefreshPlayerSources();
    }
}

void UMatchMapHUD::DebugRoundSubState(const FString& RoundSubStateStr)
{
}

void UMatchMapHUD::InitializeAbilityStacks()
{
    RefreshPlayerSources();
}

void UMatchMapHUD::UpdateSlotStackInfo(EAbilitySlotType AbilitySlot, int32 AbilityID)
{
    if (AbilityBar && bHudConstructed)
    {
        AbilityBar->RefreshSlot(AbilitySlot, AbilityID);
    }
}

FHUDAbilityInfo UMatchMapHUD::GetAbilityInfo(int32 AbilityID) const
{
    return AbilityBar ? AbilityBar->GetAbility(AbilityID) : FHUDAbilityInfo();
}

FHUDAbilityInfo UMatchMapHUD::GetAbilityInfoBySlot(EAbilitySlotType SlotType) const
{
    return AbilityBar ? AbilityBar->GetSlot(SlotType) : FHUDAbilityInfo();
}

UImage* UMatchMapHUD::GetAbilityImage(EAbilitySlotType AbilitySlot) const
{
    switch (AbilitySlot)
    {
    case EAbilitySlotType::Slot_C: return AbilityC_Image;
    case EAbilitySlotType::Slot_Q: return AbilityQ_Image;
    case EAbilitySlotType::Slot_E: return AbilityE_Image;
    case EAbilitySlotType::Slot_X: return AbilityX_Image;
    default: return nullptr;
    }
}

void UMatchMapHUD::CacheAbilityWidgets()
{
    const struct
    {
        EAbilitySlotType Slot;
        UTextBlock* Label;
        UCanvasPanel* Pips;
    } widgets[] = {
        { EAbilitySlotType::Slot_C, TextBlock_101, CanvasPanel_1 },
        { EAbilitySlotType::Slot_Q, TextBlock, CanvasPanel_0 },
        { EAbilitySlotType::Slot_E, TextBlock_1, CanvasPanel },
        { EAbilitySlotType::Slot_X, TextBlock_2, CanvasPanel_2 },
    };
    for (const auto& widget : widgets)
    {
        if (auto* icon = GetAbilityImage(widget.Slot))
        {
            AuthoredAbilityBrushes.Add(widget.Slot, icon->GetBrush());
        }
        if (widget.Label)
        {
            AbilityKeyLabels.Add(widget.Slot, widget.Label);
            AuthoredAbilityKeys.Add(widget.Slot, widget.Label->GetText());
        }
        if (widget.Pips)
        {
            AbilityPipPanels.Add(widget.Slot, widget.Pips);
        }
        if (!widget.Label || !widget.Pips)
        {
            UE_LOG(LogMatchHUD, Warning, TEXT("%s missing ability bindings for slot %d (label=%d, panel=%d)"),
                *GetClass()->GetName(), static_cast<int32>(widget.Slot), widget.Label != nullptr, widget.Pips != nullptr);
        }
    }
}

void UMatchMapHUD::RestoreAbilityWidgets()
{
    for (const auto slot : UHUDAbilityBarModel::SlotTypes)
    {
        if (auto* label = AbilityKeyLabels.FindRef(slot).Get())
        {
            label->SetText(AuthoredAbilityKeys.FindRef(slot));
        }
        if (auto* icon = GetAbilityImage(slot))
        {
            if (const auto* brush = AuthoredAbilityBrushes.Find(slot))
            {
                icon->SetBrush(*brush);
            }
        }
    }
    AbilityKeyLabels.Reset();
    AbilityPipPanels.Reset();
    AuthoredAbilityKeys.Reset();
    AuthoredAbilityBrushes.Reset();
}

void UMatchMapHUD::OnAbilitySlotChanged(EAbilitySlotType AbilitySlot)
{
    RenderAbilitySlot(AbilitySlot, false);
}

void UMatchMapHUD::RenderAbilitySlot(EAbilitySlotType AbilitySlot, bool bLayoutChanged)
{
    const FHUDAbilityInfo ability = AbilityBar->GetSlot(AbilitySlot);
    const bool bConfigured = ability.AbilityID > 0;
    FAbilitySlotStackInfo stack;
    stack.Initialize(ability.AbilityID, ability.CurrentStack, ability.MaxStack);
    SlotStackInfoMap.Add(AbilitySlot, stack);

    switch (AbilitySlot)
    {
    case EAbilitySlotType::Slot_C: SkillCID = ability.AbilityID; break;
    case EAbilitySlotType::Slot_Q: SkillQID = ability.AbilityID; break;
    case EAbilitySlotType::Slot_E: SkillEID = ability.AbilityID; break;
    case EAbilitySlotType::Slot_X: SkillXID = ability.AbilityID; break;
    default: break;
    }

    if (bLayoutChanged)
    {
        if (auto* icon = GetAbilityImage(AbilitySlot))
        {
            if (ability.AbilityIcon)
            {
                icon->SetBrushFromTexture(ability.AbilityIcon);
            }
            else if (const auto* brush = AuthoredAbilityBrushes.Find(AbilitySlot))
            {
                icon->SetBrush(*brush);
            }
            icon->SetRenderOpacity(bConfigured ? 1.0f : UnavailableSkillOpacity);
        }
        if (auto* pips = AbilityPipPanels.FindRef(AbilitySlot).Get())
        {
            pips->SetVisibility(bConfigured ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
        }
        InitializeAbilityMaxStacks(AbilitySlot, ability.MaxStack);
    }
    OnSlotStackInfoUpdated(AbilitySlot, stack);
    OnAbilityInfoUpdated(AbilitySlot, ability);

    if (auto* label = AbilityKeyLabels.FindRef(AbilitySlot).Get())
    {
        const FText key = AuthoredAbilityKeys.FindRef(AbilitySlot);
        label->SetText(bConfigured
            ? FText::Format(LOCTEXT("AbilityStackCount", "{0}  {1}/{2}"), key,
                FText::AsNumber(ability.CurrentStack), FText::AsNumber(ability.MaxStack))
            : FText::Format(LOCTEXT("UnavailableAbilitySlot", "{0}  —"), key));
        label->SetColorAndOpacity(bConfigured && ability.CurrentStack > 0 ? SecondaryTextColor : UnavailableSkillColor);
    }
    UE_LOG(LogMatchHUD, Verbose, TEXT("HUD ability slot=%d id=%d stacks=%d/%d"),
        static_cast<int32>(AbilitySlot), ability.AbilityID, ability.CurrentStack, ability.MaxStack);
}

void UMatchMapHUD::OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& KillFeedInfo)
{
	TopWidget->OnKillEvent(InstigatorAgent, VictimAgent, KillFeedInfo);
	const auto* MyController = GetOwningPlayer<AAgentPlayerController>();
	auto* GI = GetWorld()->GetGameInstance<UValorantGameInstance>();
	if (nullptr == GI)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, GameInstance is nullptr"), __FUNCTION__);
		return;
	}
	
	if (nullptr == MyController)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MyController is nullptr"), __FUNCTION__);
		return;
	}

	const auto* MyPS = MyController->GetPlayerState<AAgentPlayerState>();
	if (nullptr == MyPS)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MyPS is nullptr"), __FUNCTION__);
		return;
	}

	FString InstigatorName = "";
	bool bInstigatorIsMyTeam = false;
	const UTexture2D* InstigatorIcon = nullptr;
	if (InstigatorAgent)
	{
		InstigatorName = InstigatorAgent->GetPlayerNickname();
		bInstigatorIsMyTeam = InstigatorAgent->IsBlueTeam() == MyPS->bIsBlueTeam;
		if (const FAgentData* InstigatorData = GI->GetAgentData(InstigatorAgent->GetAgentID()))
		{
			// NET_LOG(LogTemp, Warning, TEXT("%hs Called, AgentID is %d, KillFeedIconName is %s"), __FUNCTION__, InstigatorAgent->GetAgentID(), *InstigatorData->KillFeedIcon->GetName());
			InstigatorIcon = InstigatorData->KillFeedIcon;
		}
	}

	FString VictimName = "";
	bool bVictimIsMyTeam = false;
	const UTexture2D* VictimIcon = nullptr;
	if (VictimAgent)
	{
		VictimName = VictimAgent->GetPlayerNickname();
		bVictimIsMyTeam = VictimAgent->IsBlueTeam() == MyPS->bIsBlueTeam;
		if (const FAgentData* VictimData = GI->GetAgentData(VictimAgent->GetAgentID()))
		{
			VictimIcon = VictimData->KillFeedIcon;
		}
	}
	
	const UTexture2D* ReasonIcon = nullptr;
	const int WeaponID = KillFeedInfo.WeaponID;
	if (const auto* Data = GI->GetWeaponData(WeaponID))
	{
		ReasonIcon = Data->WeaponIcon;
	}
	
	DisplayKillFeed(InstigatorIcon, InstigatorName, bInstigatorIsMyTeam, VictimIcon, VictimName, bVictimIsMyTeam, KillFeedInfo, ReasonIcon);
}

void UMatchMapHUD::DisplayFollowUpInputUI(FGameplayTag slotTag, EFollowUpInputType inputType)
{
    HideFollowUpInputUI();

    UPanelWidget* left = nullptr;
    UPanelWidget* either = nullptr;
    if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")))
    {
        left = Left_E;
        either = LeftOrRight_E;
    }
    else if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")))
    {
        left = Left_Q;
        either = LeftOrRight_Q;
    }
    else if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")))
    {
        left = Left_C;
        either = LeftOrRight_C;
    }

    if (inputType == EFollowUpInputType::LeftClick)
    {
        DisplayedFollowUpInputUI = left;
    }
    else if (inputType == EFollowUpInputType::LeftOrRight)
    {
        DisplayedFollowUpInputUI = either;
    }
    if (DisplayedFollowUpInputUI)
    {
        DisplayedFollowUpInputUI->SetVisibility(ESlateVisibility::HitTestInvisible);
        UE_LOG(LogMatchHUD, Verbose, TEXT("HUD follow-up=%s"), *DisplayedFollowUpInputUI->GetName());
    }
}

void UMatchMapHUD::HideFollowUpInputUI()
{
	if (DisplayedFollowUpInputUI)
	{
		DisplayedFollowUpInputUI->SetVisibility(ESlateVisibility::Hidden);
		DisplayedFollowUpInputUI = nullptr;
	}
}

int32 UMatchMapHUD::GetAbilityStack(int32 AbilityID) const
{
    return AbilityBar ? AbilityBar->GetCurrentStack(AbilityID) : 0;
}

int32 UMatchMapHUD::GetMaxAbilityStack(int32 AbilityID) const
{
    return AbilityBar ? AbilityBar->GetMaxStack(AbilityID) : 0;
}

#undef LOCTEXT_NAMESPACE
