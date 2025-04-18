// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentBaseWidget.h"

#include "GameplayTagContainer.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Player/AgentPlayerController.h"
#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Player/AgentPlayerState.h"

void UAgentBaseWidget::BindToDelegatePC(AAgentPlayerController* pc)
{
	pc->OnHealthChanged_PC.AddDynamic(this, &UAgentBaseWidget::UpdateDisplayHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &UAgentBaseWidget::UpdateDisplayArmor);
	pc->OnMoveSpeedChanged_PC.AddDynamic(this, &UAgentBaseWidget::UpdateDisplaySpeed);

	if (ASC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AgentWidget, ASC NULL"));
		return;
	}

	txt_AgentName->SetText(FText::FromString(ASC->GetAgentName()));
	
	txt_C->SetText(FText::FromString(ASC->GetSkillCName()));
	txt_E->SetText(FText::FromString(ASC->GetSkillEName()));
	txt_Q->SetText(FText::FromString(ASC->GetSkillQName()));
	txt_X->SetText(FText::FromString(ASC->GetSkillXName()));
}

void UAgentBaseWidget::UpdateDisplayHealth(const float health)
{
	txt_HP->SetText(FText::AsNumber(health));
}

void UAgentBaseWidget::UpdateDisplayArmor(const float armor)
{
	txt_Armor->SetText(FText::AsNumber(armor));
}

void UAgentBaseWidget::UpdateDisplaySpeed(const float speed)
{
	txt_Speed->SetText(FText::AsNumber(speed));
}
