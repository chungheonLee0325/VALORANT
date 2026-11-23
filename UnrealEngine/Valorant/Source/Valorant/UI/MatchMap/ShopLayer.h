#pragma once

#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "ShopViewModel.h"
#include "ShopLayer.generated.h"

class UBorder;
class UButton;
class UCommonActivatableWidgetStack;
class UMatchMapShopUI;
class UOverlay;
class UTextBlock;

UCLASS()
class UShopButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UShopButtonStyle();
};

// Behavior of a modal button. Layout, fonts and colors live in WBP_ShopButton.
UCLASS(Abstract)
class UShopButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UShopButton();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;
	virtual void UpdateInputActionWidget() override;

private:
	void RefreshAppearance();

	UPROPERTY(EditAnywhere, Category = "Shop")
	FText ButtonText;

	UPROPERTY(EditAnywhere, Category = "Shop")
	FLinearColor BackgroundColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, Category = "Shop")
	FLinearColor HighlightedBackgroundColor = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, Category = "Shop")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Shop")
	FLinearColor HighlightedTextColor = FLinearColor::White;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Label;

	// Shows the bound key as text when the platform has no icon for it.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InputKeyLabel;
};

UCLASS(Abstract)
class UShopPanel : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};

class UShopConfirmation;

UCLASS()
class UShopScreen : public UShopPanel
{
	GENERATED_BODY()

public:
	UShopScreen();
	void Configure(UShopViewModel& Model, UCommonActivatableWidgetStack& Stack, TSubclassOf<UMatchMapShopUI> ContentClass,
		TSubclassOf<UShopConfirmation> InConfirmationClass);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void CollectPurchaseButtons();
	void MoveCategory(int32 Direction);
	void OpenConfirmation(EShopItemType Type, int32 ItemId);
	void ApplyPurchaseOutcome();
	UButton* FindFocusedButton() const;
	void OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field);

	UPROPERTY(Transient)
	TObjectPtr<UShopViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> ShopStack;

	UPROPERTY(Transient)
	TSubclassOf<UShopConfirmation> ConfirmationClass;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ContentRoot;

	UPROPERTY(Transient)
	TObjectPtr<UMatchMapShopUI> Content;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> WeaponButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> AbilityButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ArmorButtons;
};

// Behavior of the purchase modal. Layout, fonts and colors live in WBP_ShopConfirmation.
UCLASS(Abstract)
class UShopConfirmation : public UShopPanel
{
	GENERATED_BODY()

public:
	void Configure(UShopViewModel& Model);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void Confirm();

	UPROPERTY(Transient)
	TObjectPtr<UShopViewModel> ViewModel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Details;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UShopButton> ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UShopButton> CancelButton;
};

UCLASS()
class UShopLayer : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(UShopViewModel& ViewModel, TSubclassOf<UMatchMapShopUI> ContentClass, TSubclassOf<UShopConfirmation> ConfirmationClass);
	void Close();
	bool IsOpen() const;

protected:
	virtual void NativeOnInitialized() override;

private:
	void OnDisplayedWidgetChanged(UCommonActivatableWidget* Widget);

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> Stack;
};
