// Fill out your copyright notice in the Description page of Project Settings.


#include "DetectWidget.h"

#include "Components/TextBlock.h"

void UDetectWidget::SetName(const FString& Name) const
{
	TextBlockName->SetText(FText::FromString(Name));
}
