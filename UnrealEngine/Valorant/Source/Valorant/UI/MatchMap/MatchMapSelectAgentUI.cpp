// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapSelectAgentUI.h"

#include "Valorant.h"
#include "Components/ButtonSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CustomWidget/AgentSelectButton.h"
#include "CustomWidget/TeamSelectAgentBox.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/AgentPlayerController.h"

TMap<FString, TPair<FString, FString>> AgentRoleDescriptionMap = {
	{ TEXT("Duelist"), { TEXT("타격대"), TEXT("타격대는 자급자족이 가능하며 팀에서 공격을 담당합니다. 스킬과 실력을 통해 먼저 교전을 시작하고 적을 처치합니다.") } },
	{ TEXT("Initiator"), { TEXT("척후대"), TEXT("척후대는 팀이 전투 지역에 침입할 수 있도록 다각도로 진입로를 확보하고 적 팀에 틈을 만들어냅니다.") } },
	{ TEXT("Sentinel"), { TEXT("감시자"), TEXT("감시자들은 공격팀이나 수비팀에 상관없이 지역을 점유하고 팀 엄호를 담당하는 수비 전문가입니다.") } },
	{ TEXT("Controller"), { TEXT("전략가"), TEXT("전략가들은 위험 지역을 분석하여 팀을 승리로 이끄는 데 탁월합니다.") } }
};

void UMatchMapSelectAgentUI::NativeConstruct()
{
	Super::NativeConstruct();

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapSelectAgentUI::UpdateTime);
	GetOwningPlayer()->SetShowMouseCursor(true);
	FillAgentList();

#ifdef DEBUGTEST
	if (false == GetOwningPlayer()->HasAuthority())
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			auto* Controller = Cast<AAgentPlayerController>(GetOwningPlayer());
			Controller->ServerRPC_LockIn(2);
		},1.0f,false);
	}
#endif
}

void UMatchMapSelectAgentUI::NativeDestruct()
{
	Super::NativeDestruct();
	GetOwningPlayer()->SetShowMouseCursor(false);
}

void UMatchMapSelectAgentUI::OnClickedButtonLockIn()
{
	if (CurrentSelectedAgentID == 0)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Abort LockIn. Because CurrentSelectedAgentID is 0"), __FUNCTION__);
		return;
	}
	
	auto* Controller = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (nullptr == Controller)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Controller is nullptr"), __FUNCTION__);
		return;
	}
	
	PlayAgentSelectSound(CurrentSelectedAgentID);
	Controller->ServerRPC_LockIn(CurrentSelectedAgentID);
}

void UMatchMapSelectAgentUI::OnClickedAgentSelectButton(int AgentId)
{
	if (AgentId == 5)
	{
		return;
	}
	
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, AgentId: %d"), __FUNCTION__, AgentId);
	CurrentSelectedAgentID = AgentId;
	auto NewStyle = ButtonLockIn->GetStyle();
	NewStyle.Normal.TintColor = FSlateColor(FLinearColor(0.556863f, 0.050980f, 0.090196f, 1.0f));
	ButtonLockIn->SetStyle(NewStyle);
	ButtonLockIn->SetIsEnabled(true);
	VerticalBoxRight->SetVisibility(ESlateVisibility::Visible);
	OnClickAgentSelectButtonDelegate.Broadcast(AgentId);
}

void UMatchMapSelectAgentUI::UpdateTime(float Time)
{
	if (nullptr == TextBlockRemTime)
	{
		return;
	}
	TextBlockRemTime->SetText(FText::FromString(FString::Printf(TEXT("%d"), static_cast<int>(Time))));
}

void UMatchMapSelectAgentUI::FillAgentList()
{
	auto* GameInstance = GetGameInstance<UValorantGameInstance>();
	GridPanelAgentList->ClearChildren();
	int Idx = 1;
	for (int Row = 0; Row < 3; ++Row)
	{
		bool bBreak = false;
		for (int Col = 0; Col < 4; ++Col)
		{
			const auto* Data = GameInstance->GetAgentData(Idx++);
			if (nullptr == Data)
			{
				bBreak = true;
				break;
			}
			UAgentSelectButton* AgentButton = NewObject<UAgentSelectButton>(this);
			AgentButton->Init(Data->AgentID);
			AgentButton->OnAgentSelectButtonClicked.AddDynamic(this, &UMatchMapSelectAgentUI::OnClickedAgentSelectButton);
			auto* GridSlot = GridPanelAgentList->AddChildToGrid(AgentButton, Row, Col);
			
			FMargin Margin;
			if (Row != 0) Margin.Top = 10.f;
			if (Col != 0) Margin.Left = 10.f;
			GridSlot->SetPadding(Margin);
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
			
			FButtonStyle Style;
			Style.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
			Style.Normal.TintColor = FSlateColor(FLinearColor(0.495466f, 0.495466f, 0.495466f, 0.1f));
			Style.Normal.OutlineSettings.Color = FSlateColor(FLinearColor::White);
			Style.Normal.OutlineSettings.CornerRadii = FVector4(0.f);
			Style.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Style.Normal.OutlineSettings.Width = 1.f;
			Style.Normal.OutlineSettings.bUseBrushTransparency = false;
			Style.NormalPadding = FMargin(5.f);
			
			Style.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
			Style.Hovered.TintColor = FSlateColor(FLinearColor(0.724268f, 0.724268f, 0.724268f, 1.0f));
			Style.Hovered.OutlineSettings.Color = FSlateColor(FLinearColor::White);
			Style.Hovered.OutlineSettings.CornerRadii = FVector4(4.f);
			Style.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Style.Hovered.OutlineSettings.Width = 1.f;
			Style.Hovered.OutlineSettings.bUseBrushTransparency = true;

			Style.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
			Style.Pressed.TintColor = FSlateColor(FLinearColor(0.384266f, 0.384266f, 0.384266f, 1.0f));
			Style.Pressed.OutlineSettings.Color = FSlateColor(FLinearColor::White);
			Style.Pressed.OutlineSettings.CornerRadii = FVector4(4.f);
			Style.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Style.Pressed.OutlineSettings.Width = 1.f;
			Style.Pressed.OutlineSettings.bUseBrushTransparency = true;
			Style.PressedPadding = FMargin(5.f);
			
			AgentButton->SetStyle(Style);
			
			const FString& AgentName = Data->AgentName;
			UImage* ThumbImage = NewObject<UImage>(this);
			UTexture2D* Texture = Cast<UTexture2D>(
				StaticLoadObject(
					UTexture2D::StaticClass(),
					nullptr,
					*FString::Printf(TEXT("/Game/Resource/UI/Shared/Icons/Character/Thumbnails/TX_Character_Thumb_%s.TX_Character_Thumb_%s"), *AgentName, *AgentName)
				)
			);

			FSlateBrush Brush;
			Brush.SetResourceObject(Texture);
			Brush.ImageSize = FVector2D(96.5f, 96.5f);
			Brush.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
			Brush.DrawAs = ESlateBrushDrawType::Image;
			ThumbImage->SetBrush(Brush);
			UButtonSlot* ButtonSlot = Cast<UButtonSlot>(AgentButton->AddChild(ThumbImage));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
			ButtonSlot->SetPadding(FMargin(0.f));
		}
		if (bBreak) break;
	}
}

void UMatchMapSelectAgentUI::FillTeamSelectAgentList(const TArray<FString>& TeamPlayerNameArray)
{
	for (const auto& PlayerName : TeamPlayerNameArray)
	{
		AddTeamBox(PlayerName);
	}
}

void UMatchMapSelectAgentUI::OnSelectedAgentChanged(const FString& DisplayName, int SelectedAgentID)
{
	if (false == TeamSelectAgentBoxMap.Contains(DisplayName))
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, WhoAreYou?? DisplayName: %s"), __FUNCTION__, *DisplayName);
		return;
	}
	
	auto* GameInstance = GetGameInstance<UValorantGameInstance>();
	const auto* Data = GameInstance->GetAgentData(SelectedAgentID);
	if (Data)
	{
		const FString& RoleName = StaticEnum<EAgentRole>()->GetNameStringByValue(static_cast<int>(Data->AgentRole));
		const FString& AgentName = Data->LocalName;
		const FString& AgentDescription = Data->Description;
		const auto& RoleDescription = AgentRoleDescriptionMap.FindChecked(RoleName);
		TextBlockPosition1->SetText(FText::FromString(RoleDescription.Key));
		TextBlockAgentName->SetText(FText::FromString(AgentName));
		TextBlockAgentDescription->SetText(FText::FromString(AgentDescription));
		TextBlockPosition2->SetText(FText::FromString(RoleDescription.Key));
		TextBlockPositionDescription->SetText(FText::FromString(RoleDescription.Value));
	}
	TeamSelectAgentBoxMap[DisplayName]->ChangeAgentThumbImage(SelectedAgentID);
}


void UMatchMapSelectAgentUI::OnLockIn(const FString& DisplayName, const int AgentId)
{
	if (false == TeamSelectAgentBoxMap.Contains(DisplayName))
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, WhoAreYou?? DisplayName: %s"), __FUNCTION__, *DisplayName);
		return;
	}
	if (0 == AgentId)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Agent is not selected"), __FUNCTION__);
		return;
	}
	TeamSelectAgentBoxMap[DisplayName]->LockIn(AgentId);
}