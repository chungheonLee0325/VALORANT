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

UCLASS()
class UShopButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	void SetContent(const FText& Title);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	void RefreshAppearance();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Background;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;
};

UCLASS(Abstract)
class UShopPanel : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
};

UCLASS()
class UShopScreen : public UShopPanel
{
	GENERATED_BODY()

public:
	void Configure(UShopViewModel& Model, UCommonActivatableWidgetStack& Stack, TSubclassOf<UMatchMapShopUI> ContentClass);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

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
	TObjectPtr<UOverlay> ContentRoot;

	UPROPERTY(Transient)
	TObjectPtr<UMatchMapShopUI> Content;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> WeaponButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> AbilityButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ArmorButtons;

	TWeakObjectPtr<UButton> RestoreFocus;
	FDelegateHandle FocusChangingHandle;
};

UCLASS()
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

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Details;

	UPROPERTY(Transient)
	TObjectPtr<UShopButton> ConfirmButton;
};

UCLASS()
class UShopLayer : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(UShopViewModel& ViewModel, TSubclassOf<UMatchMapShopUI> ContentClass);
	void Close();
	bool IsOpen() const;

protected:
	virtual void NativeOnInitialized() override;

private:
	void OnDisplayedWidgetChanged(UCommonActivatableWidget* Widget);

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> Stack;
};
