#include "ShopLayer.h"

#include "MatchMapShopUI.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateNoResource.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/CommonUIInputTypes.h"
#include "Player/AgentPlayerController.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#define LOCTEXT_NAMESPACE "ValorantShop"

namespace
{
	const FLinearColor Accent(0.20f, 0.92f, 0.74f);

	UTextBlock* AddText(UWidgetTree& Tree, UVerticalBox& Box, const FText& Text, int32 Size)
	{
		auto* label = Tree.ConstructWidget<UTextBlock>();
		label->SetText(Text);
		label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Size));
		Box.AddChildToVerticalBox(label)->SetPadding(FMargin(0, 4));
		return label;
	}

	UVerticalBox* MakePanel(UWidgetTree& Tree, float Width)
	{
		auto* background = Tree.ConstructWidget<UBorder>();
		background->SetBrushColor(FLinearColor(0.008f, 0.015f, 0.024f, 0.90f));
		auto* center = Tree.ConstructWidget<UOverlay>();
		background->SetContent(center);
		Tree.RootWidget = background;

		auto* size = Tree.ConstructWidget<USizeBox>();
		size->SetWidthOverride(Width);
		auto* slot = center->AddChildToOverlay(size);
		slot->SetHorizontalAlignment(HAlign_Center);
		slot->SetVerticalAlignment(VAlign_Center);

		auto* panel = Tree.ConstructWidget<UBorder>();
		panel->SetBrushColor(FLinearColor(0.035f, 0.060f, 0.085f));
		panel->SetPadding(FMargin(24, 18));
		size->SetContent(panel);
		auto* body = Tree.ConstructWidget<UVerticalBox>();
		panel->SetContent(body);
		return body;
	}

	UShopButton* AddButton(UWidgetTree& Tree, UHorizontalBox& Row, const FText& Text)
	{
		auto* button = Tree.ConstructWidget<UShopButton>();
		button->SetContent(Text);
		auto* slot = Row.AddChildToHorizontalBox(button);
		slot->SetPadding(FMargin(0, 0, 5, 0));
		slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		return button;
	}
}

UShopButtonStyle::UShopButtonStyle()
{
	NormalBase = NormalHovered = NormalPressed = FSlateNoResource();
	SelectedBase = SelectedHovered = SelectedPressed = Disabled = FSlateNoResource();
	ButtonPadding = FMargin(0);
}

void UShopButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetPadding(FMargin(12, 9));
	Background->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Background;
	Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	Background->SetContent(Label);
	SetStyle(UShopButtonStyle::StaticClass());
	OnFocusReceived().AddUObject(this, &UShopButton::RefreshAppearance);
	OnFocusLost().AddUObject(this, &UShopButton::RefreshAppearance);
	OnHovered().AddUObject(this, &UShopButton::RefreshAppearance);
	OnUnhovered().AddUObject(this, &UShopButton::RefreshAppearance);
	RefreshAppearance();
}

void UShopButton::SetContent(const FText& Title)
{
	Label->SetText(Title);
}

FReply UShopButton::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey key = Event.GetKey();
	if (key == EKeys::Enter || key == EKeys::Virtual_Accept || key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (!Event.IsRepeat())
		{
			HandleButtonClicked();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

void UShopButton::RefreshAppearance()
{
	auto* player = GetOwningPlayer();
	const bool bHighlighted = IsHovered()
		|| (player && (HasUserFocus(player) || HasUserFocusedDescendants(player)));
	Background->SetBrushColor(bHighlighted ? FLinearColor(0.08f, 0.28f, 0.30f) : FLinearColor(0.07f, 0.12f, 0.16f));
	Label->SetColorAndOpacity(bHighlighted ? Accent : FLinearColor::White);
}

TOptional<FUIInputConfig> UShopPanel::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock);
}

FReply UShopPanel::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey key = Event.GetKey();
	if (IsActivated() && (key == EKeys::Escape || key == EKeys::Gamepad_FaceButton_Right || key == EKeys::B
		|| key == EKeys::Gamepad_Special_Left))
	{
		if (!Event.IsRepeat())
		{
			DeactivateWidget();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UShopScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ContentRoot = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = ContentRoot;
}

void UShopScreen::Configure(UShopViewModel& Model, UCommonActivatableWidgetStack& Stack,
	TSubclassOf<UMatchMapShopUI> ContentClass)
{
	ViewModel = &Model;
	ShopStack = &Stack;
	RestoreFocus.Reset();
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
	FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddWeakLambda(this,
		[this](const FFocusEvent&, const FWeakWidgetPath&, const TSharedPtr<SWidget>&,
			const FWidgetPath&, const TSharedPtr<SWidget>& focused)
		{
			if (!focused.IsValid())
			{
				return;
			}
			for (const auto* group : {&WeaponButtons, &AbilityButtons, &ArmorButtons})
			{
				for (const auto& button : *group)
				{
					if (button->GetCachedWidget() == focused)
					{
						RestoreFocus = button.Get();
						return;
					}
				}
			}
		});
	Content->UpdateCreditDisplay(ViewModel->CurrentCredits);
	Content->InitializeAbilityUI();
	Content->UpdateWeaponHighlights();
	ApplyPurchaseOutcome();
}

void UShopScreen::NativeOnDeactivated()
{
	if (FocusChangingHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnFocusChanging().Remove(FocusChangingHandle);
		FocusChangingHandle.Reset();
	}
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
	if (RestoreFocus.IsValid() && RestoreFocus->GetIsEnabled() && RestoreFocus->IsVisible())
	{
		return RestoreFocus.Get();
	}
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

FReply UShopScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey key = Event.GetKey();
	if (IsActivated() && (key == EKeys::Gamepad_LeftShoulder || key == EKeys::Gamepad_RightShoulder))
	{
		if (!Event.IsRepeat())
		{
			MoveCategory(key == EKeys::Gamepad_RightShoulder ? 1 : -1);
		}
		return FReply::Handled();
	}
	if (IsActivated() && (key == EKeys::Enter || key == EKeys::Gamepad_FaceButton_Bottom))
	{
		auto* button = FindFocusedButton();
		if (!button)
		{
			button = RestoreFocus.Get();
		}
		if (button && button->GetIsEnabled() && button->IsVisible())
		{
			if (!Event.IsRepeat())
			{
				button->OnClicked.Broadcast();
			}
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
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
			RestoreFocus = target->Get();
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
			if (auto* focused = FindFocusedButton())
			{
				RestoreFocus = focused;
			}
			ShopStack->AddWidget<UShopConfirmation>(UShopConfirmation::StaticClass(), [this](UShopConfirmation& dialog)
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
	auto* body = MakePanel(*WidgetTree, 460);
	AddText(*WidgetTree, *body, LOCTEXT("ConfirmTitle", "구매 확인"), 26);
	Details = AddText(*WidgetTree, *body, FText::GetEmpty(), 18);
	auto* actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	body->AddChildToVerticalBox(actions)->SetPadding(FMargin(0, 12, 0, 0));
	ConfirmButton = AddButton(*WidgetTree, *actions, LOCTEXT("Confirm", "구매"));
	ConfirmButton->OnClicked().AddUObject(this, &UShopConfirmation::Confirm);
	auto* cancel = AddButton(*WidgetTree, *actions, LOCTEXT("Cancel", "취소"));
	cancel->OnClicked().AddUObject(this, &UShopConfirmation::DeactivateWidget);
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

void UShopLayer::Open(UShopViewModel& ViewModel, TSubclassOf<UMatchMapShopUI> ContentClass)
{
	if (!IsOpen())
	{
		Stack->SetTransitionDuration(0);
		Stack->AddWidget<UShopScreen>(UShopScreen::StaticClass(), [this, &ViewModel, ContentClass](UShopScreen& screen)
		{
			screen.Configure(ViewModel, *Stack, ContentClass);
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
