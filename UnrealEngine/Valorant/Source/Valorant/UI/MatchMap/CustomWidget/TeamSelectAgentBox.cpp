// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamSelectAgentBox.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameManager/ValorantGameInstance.h"

void UTeamSelectAgentBox::ChangeAgentThumbImage(const int AgentId)
{
	auto* GameInstance = GetGameInstance<UValorantGameInstance>();
	const auto* Data = GameInstance->GetAgentData(AgentId);
	if (nullptr == Data)
	{
		return;
	}
	const FString& AgentName = Data->AgentName;
	UTexture2D* Texture = Cast<UTexture2D>(
		StaticLoadObject(
			UTexture2D::StaticClass(),
			nullptr,
			*FString::Printf(TEXT("/Game/Resource/UI/Shared/Icons/Character/Thumbnails/TX_Character_Thumb_%s.TX_Character_Thumb_%s"), *AgentName, *AgentName)
		)
	);

	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(NoiseMaterial, this);
	DynamicMaterial->SetTextureParameterValue(FName("ThumbnailTexture"), Texture);
	
	FSlateBrush Brush = ImageAgentThumb->GetBrush();
	Brush.SetResourceObject(DynamicMaterial);
	Brush.ImageSize = FVector2D(128.f);
	Brush.TintColor = FSlateColor(FLinearColor(0.744792f, 0.8878f, 1.f, 0.8f));
	ImageAgentThumb->SetBrush(Brush);
	
	BorderThumb->SetBrushColor(FLinearColor(0.144821f, 0.406411f, 0.541667f, 0.4f));
}

void UTeamSelectAgentBox::LockIn(const int AgentId)
{
	auto* GameInstance = GetGameInstance<UValorantGameInstance>();
	const auto* Data = GameInstance->GetAgentData(AgentId);
	if (nullptr == Data)
	{
		return;
	}
	const FString& AgentName = Data->AgentName;
	const FString& LocalName = Data->LocalName;
	UTexture2D* Texture = Cast<UTexture2D>(
		StaticLoadObject(
			UTexture2D::StaticClass(),
			nullptr,
			*FString::Printf(TEXT("/Game/Resource/UI/Shared/Icons/Character/Thumbnails/TX_Character_Thumb_%s.TX_Character_Thumb_%s"), *AgentName, *AgentName)
		)
	);
	FSlateBrush Brush = ImageAgentThumb->GetBrush();
	Brush.SetResourceObject(Texture);
	Brush.TintColor = FSlateColor(FLinearColor::White);
	ImageAgentThumb->SetBrush(Brush);
	
	TextStatus->SetText(FText::FromString(LocalName));
	TextStatus->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	BorderThumb->SetBrushColor(FLinearColor(1, 1, 1, 0));
}
