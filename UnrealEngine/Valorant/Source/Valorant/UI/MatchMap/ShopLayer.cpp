#include "ShopLayer.h"

#include "MatchMapShopUI.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateNoResource.h"
#include "CommonActionWidget.h"
#include "CommonInputSettings.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Player/AgentPlayerController.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#define LOCTEXT_NAMESPACE "ValorantShop"

namespace
{
	// Shop actions are rows of the UI action table that holds the project's default Back action.
	FDataTableRowHandle ShopAction(FName Row)
	{
		FDataTableRowHandle action = ICommonInputModule::GetSettings().GetDefaultBackAction();
		action.RowName = Row;
		return action;
	}
}

UShopButtonStyle::UShopButtonStyle()
{
	NormalBase = NormalHovered = NormalPressed = FSlateNoResource();
	SelectedBase = SelectedHovered = SelectedPressed = Disabled = FSlateNoResource();
	ButtonPadding = FMargin(0);
}

UShopButton::UShopButton()
{
	Style = UShopButtonStyle::StaticClass();
}

void UShopButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	// The designer previews this before every bound widget exists.
	if (Label && Background)
	{
		Label->SetText(ButtonText);
		RefreshAppearance();
	}
}

void UShopButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	OnFocusReceived().AddUObject(this, &UShopButton::RefreshAppearance);
	OnFocusLost().AddUObject(this, &UShopButton::RefreshAppearance);
	OnHovered().AddUObject(this, &UShopButton::RefreshAppearance);
	OnUnhovered().AddUObject(this, &UShopButton::RefreshAppearance);
}

void UShopButton::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();
	auto* input = GetInputSubsystem();
	if (!InputKeyLabel || !input)
	{
		return;
	}
	const bool bHasIcon = InputActionWidget && InputActionWidget->GetIcon().DrawAs != ESlateBrushDrawType::NoDrawType;
	const auto* action = TriggeringInputAction.GetRow<FCommonInputActionDataBase>(TEXT("ShopButton"));
	const FKey key = action && !bHasIcon
		? action->GetInputTypeInfo(input->GetCurrentInputType(), input->GetCurrentGamepadName()).GetKey() : FKey();
	InputKeyLabel->SetText(key.IsValid() ? key.GetDisplayName(false) : FText::GetEmpty());
	InputKeyLabel->SetVisibility(key.IsValid() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UShopButton::RefreshAppearance()
{
	auto* player = GetOwningPlayer();
	const bool bHighlighted = IsHovered()
		|| (player && (HasUserFocus(player) || HasUserFocusedDescendants(player)));
	Background->SetBrushColor(bHighlighted ? HighlightedBackgroundColor : BackgroundColor);
	Label->SetColorAndOpacity(bHighlighted ? HighlightedTextColor : TextColor);
}

void UShopPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RegisterUIActionBinding(FBindUIActionArgs(ShopAction(TEXT("ShopClose")), false, FSimpleDelegate::CreateUObject(this, &UShopPanel::DeactivateWidget)));
}

TOptional<FUIInputConfig> UShopPanel::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock);
}

UShopScreen::UShopScreen()
{
	bIsBackHandler = true;
	bAutoRestoreFocus = true;
}

void UShopScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ContentRoot = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = ContentRoot;
	RegisterUIActionBinding(FBindUIActionArgs(ShopAction(TEXT("ShopPreviousCategory")), false, FSimpleDelegate::CreateUObject(this, &UShopScreen::MoveCategory, -1)));
	RegisterUIActionBinding(FBindUIActionArgs(ShopAction(TEXT("ShopNextCategory")), false, FSimpleDelegate::CreateUObject(this, &UShopScreen::MoveCategory, 1)));
}

void UShopScreen::Configure(UShopViewModel& Model, UCommonActivatableWidgetStack& Stack,
	TSubclassOf<UMatchMapShopUI> ContentClass, TSubclassOf<UShopConfirmation> InConfirmationClass)
{
	ViewModel = &Model;
	ShopStack = &Stack;
	ConfirmationClass = InConfirmationClass;
	// A reopened shop starts from its first item instead of the last focused one.
	ClearFocusRestorationTarget();
	if (!Content || Content->GetClass() != ContentClass)
	{
		ContentRoot->ClearChildren();
		Content = CreateWidget<UMatchMapShopUI>(GetOwningPlayer(), ContentClass);
		if (!Content)
		{
			return;
		}
		Content->SetPresentationOwner(GetOwningPlayer<AAgentPlayerController>());
		Content->OnItemRequested.BindUObject(this, &UShopScreen::OpenConfirmation);
		CollectPurchaseButtons();

		auto* slot = ContentRoot->AddChildToOverlay(Content);
		slot->SetHorizontalAlignment(HAlign_Fill);
		slot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UShopScreen::CollectPurchaseButtons()
{
	WeaponButtons = Content->GetPurchaseButtons(EShopItemType::Weapon);
	AbilityButtons = Content->GetPurchaseButtons(EShopItemType::Ability);
	ArmorButtons = Content->GetPurchaseButtons(EShopItemType::Armor);
	for (auto* group : {&WeaponButtons, &AbilityButtons, &ArmorButtons})
	{
		group->RemoveAll([](const TObjectPtr<UButton>& button)
		{
			return !IsValid(button.Get()) || !button->OnClicked.IsBound() || !button->GetIsFocusable();
		});
	}
}

void UShopScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (!Content || !ViewModel)
	{
		return;
	}

	ViewModel->GetFieldNotificationDescriptor().ForEachField(ViewModel->GetClass(), [this](UE::FieldNotification::FFieldId field)
	{
		ViewModel->AddFieldValueChangedDelegate(field, UShopViewModel::FFieldValueChangedDelegate::CreateUObject(this, &UShopScreen::OnFieldChanged));
		return true;
	});
	ViewModel->OnAbilityStackUpdated.AddUObject(Content.Get(), &UMatchMapShopUI::HandleAbilityStackChanged);
	ViewModel->OnEquipmentUpdated.AddUObject(Content.Get(), &UMatchMapShopUI::UpdateWeaponHighlights);
	Content->UpdateCreditDisplay(ViewModel->CurrentCredits);
	Content->InitializeAbilityUI();
	Content->UpdateWeaponHighlights();
	ApplyPurchaseOutcome();
}

void UShopScreen::NativeOnDeactivated()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
		if (Content)
		{
			ViewModel->OnAbilityStackUpdated.RemoveAll(Content.Get());
			ViewModel->OnEquipmentUpdated.RemoveAll(Content.Get());
			Content->ClearPresentationTimers();
		}
	}
	Super::NativeOnDeactivated();
}

UWidget* UShopScreen::NativeGetDesiredFocusTarget() const
{
	for (const auto* group : {&WeaponButtons, &AbilityButtons, &ArmorButtons})
	{
		if (const auto* target = group->FindByPredicate([](const TObjectPtr<UButton>& button)
			{ return button->GetIsEnabled() && button->IsVisible(); }))
		{
			return target->Get();
		}
	}
	return Content;
}

UButton* UShopScreen::FindFocusedButton() const
{
	auto* player = GetOwningPlayer();
	if (player)
	{
		for (const auto* group : {&WeaponButtons, &AbilityButtons, &ArmorButtons})
		{
			for (const auto& button : *group)
			{
				if (button->HasUserFocus(player))
				{
					return button.Get();
				}
			}
		}
	}
	return nullptr;
}

void UShopScreen::MoveCategory(int32 Direction)
{
	const TArray<TObjectPtr<UButton>>* groups[] = {&WeaponButtons, &AbilityButtons, &ArmorButtons};
	constexpr int32 count = UE_ARRAY_COUNT(groups);
	auto* focused = FindFocusedButton();
	int32 origin = 0;
	for (int32 index = 0; index < count; ++index)
	{
		if (groups[index]->Contains(focused))
		{
			origin = index;
			break;
		}
	}

	for (int32 step = 1; step < count; ++step)
	{
		const int32 category = (origin + count + Direction * step) % count;
		if (const auto* target = groups[category]->FindByPredicate([](const TObjectPtr<UButton>& button)
			{ return button->GetIsEnabled() && button->IsVisible(); }))
		{
			(*target)->SetUserFocus(GetOwningPlayer());
			return;
		}
	}
}

void UShopScreen::OpenConfirmation(EShopItemType Type, int32 ItemId)
{
	if (!IsActivated() || !ViewModel || ViewModel->bPurchasePending)
	{
		return;
	}
	const auto items = ViewModel->GetItems(Type);
	const auto* item = items.FindByPredicate([ItemId](const FShopItem& value) { return value.ItemID == ItemId; });
	if (item)
	{
		ViewModel->SelectItem(*item);
		if (ViewModel->bCanPurchase)
		{
			ShopStack->AddWidget<UShopConfirmation>(ConfirmationClass, [this](UShopConfirmation& dialog)
			{
				dialog.Configure(*ViewModel);
			});
			return;
		}
	}
	Content->ShowUnavailable();
}

void UShopScreen::OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field)
{
	if (Field.GetName() == GET_MEMBER_NAME_CHECKED(UShopViewModel, CurrentCredits))
	{
		Content->UpdateCreditDisplay(ViewModel->CurrentCredits);
	}
	if (Field.GetName() == GET_MEMBER_NAME_CHECKED(UShopViewModel, bPurchasePending))
	{
		ApplyPurchaseOutcome();
	}
}

void UShopScreen::ApplyPurchaseOutcome()
{
	if (auto result = ViewModel->TakePurchaseOutcome())
	{
		Content->ApplyPurchaseOutcome(result->bSuccess, result->ItemID, result->ItemType, result->FailureReason);
	}
}

void UShopConfirmation::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const auto& settings = ICommonInputModule::GetSettings();
	ConfirmButton->SetTriggeringInputAction(settings.GetDefaultClickAction());
	ConfirmButton->OnClicked().AddUObject(this, &UShopConfirmation::Confirm);
	CancelButton->SetTriggeringInputAction(settings.GetDefaultBackAction());
	CancelButton->OnClicked().AddUObject(this, &UShopConfirmation::DeactivateWidget);
}

void UShopConfirmation::Configure(UShopViewModel& Model)
{
	ViewModel = &Model;
	Details->SetText(FText::Format(LOCTEXT("ConfirmDetails", "{0}   ·   {1} 크레딧"),
		FText::FromString(Model.SelectedItemName), FText::AsNumber(Model.SelectedItemPrice)));
}

UWidget* UShopConfirmation::NativeGetDesiredFocusTarget() const
{
	return ConfirmButton;
}

void UShopConfirmation::Confirm()
{
	if (IsActivated())
	{
		ViewModel->RequestPurchase();
		DeactivateWidget();
	}
}

void UShopLayer::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Stack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>();
	Stack->OnDisplayedWidgetChanged().AddUObject(this, &UShopLayer::OnDisplayedWidgetChanged);
	WidgetTree->RootWidget = Stack;
}

void UShopLayer::Open(UShopViewModel& ViewModel, TSubclassOf<UMatchMapShopUI> ContentClass, TSubclassOf<UShopConfirmation> ConfirmationClass)
{
	if (!IsOpen())
	{
		Stack->SetTransitionDuration(0);
		Stack->AddWidget<UShopScreen>(UShopScreen::StaticClass(), [this, &ViewModel, ContentClass, ConfirmationClass](UShopScreen& screen)
		{
			screen.Configure(ViewModel, *Stack, ContentClass, ConfirmationClass);
		});
	}
}

void UShopLayer::Close()
{
	Stack->ClearWidgets();
}

bool UShopLayer::IsOpen() const
{
	return Stack->GetActiveWidget() != nullptr;
}

void UShopLayer::OnDisplayedWidgetChanged(UCommonActivatableWidget* Widget)
{
	UE_LOG(LogTemp, Display, TEXT("Shop screen: %s"), *GetNameSafe(Widget));
	if (!Widget)
	{
		if (auto* pc = GetOwningPlayer<AAgentPlayerController>())
		{
			pc->RestoreShopGameplayInput();
		}
	}
}

#undef LOCTEXT_NAMESPACE
