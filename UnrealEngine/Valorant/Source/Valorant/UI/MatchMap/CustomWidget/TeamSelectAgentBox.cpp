// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamSelectAgentBox.h"

#include "Components/Image.h"
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
	FSlateBrush Brush = ImageAgentThumb->GetBrush();
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = FVector2D(128.f);
	Brush.TintColor = FSlateColor(FLinearColor(0.119745f, 0.262328f, 1.f, 1.f));
	ImageAgentThumb->SetBrush(Brush);
}

void UTeamSelectAgentBox::LockIn()
{
	FSlateBrush Brush = ImageAgentThumb->GetBrush();
	Brush.TintColor = FSlateColor(FLinearColor::White);
	ImageAgentThumb->SetBrush(Brush);
}
