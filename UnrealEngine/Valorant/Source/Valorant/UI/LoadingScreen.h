// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "LoadingScreen.generated.h"

class SLoadingScreen;
/**
 * 
 */
UCLASS()
class VALORANT_API ULoadingScreen : public UWidget
{
	GENERATED_BODY()

public:
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FSlateBrush BackgroundBrush;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	TSharedPtr<SLoadingScreen> LoadingSlate;
};
