// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentBaseWidget.h"

#include "GameplayTagContainer.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Player/AgentPlayerState.h"

void UAgentBaseWidget::InitDisplayAgentData(const int32 health, const int32 armor, const int32 speed)
{
	if (ASC==nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AgentBaseWidget: ASC is null"));
		return;
	}

	txt_AgentName->SetText(FText::FromString(ASC->GetAgentName()));
	
	txt_C->SetText(FText::FromString(ASC->GetSkillCName()));
	txt_E->SetText(FText::FromString(ASC->GetSkillEName()));
	txt_Q->SetText(FText::FromString(ASC->GetSkillQName()));
	txt_X->SetText(FText::FromString(ASC->GetSkillXName()));

	AAgentPlayerState* ps = Cast<AAgentPlayerState>(GetOwningPlayerState());
	UpdateDisplayHealth(health);
	UpdateDisplayArmor(armor);
	UpdateDisplaySpeed(speed);
}

void UAgentBaseWidget::UpdateDisplayHealth(const int32 health)
{
	txt_HP->SetText(FText::AsNumber(health));
}

void UAgentBaseWidget::UpdateDisplayArmor(const int32 armor)
{
	txt_Armor->SetText(FText::AsNumber(armor));
}

void UAgentBaseWidget::UpdateDisplaySpeed(const int32 speed)
{
	txt_Speed->SetText(FText::AsNumber(speed));
}

void UAgentBaseWidget::UpdateDisplaySkill(const FGameplayTag skillTag, const FName skillName)
{
	UE_LOG(LogTemp, Warning, TEXT("위젯업데이트 호출"));
	if (skillTag == TagC)
	{
		img_C->SetColorAndOpacity(FLinearColor(0.1f,0.2f,0.7f,0.0f));
		txt_C->SetText(FText::FromName(skillName));
	}
	else if (skillTag == TagE)
	{
		img_E->SetColorAndOpacity(FLinearColor(0.1f,0.2f,0.7f,0.0f));
		txt_E->SetText(FText::FromName(skillName));
	}
	else if (skillTag == TagQ)
	{
		img_Q->SetColorAndOpacity(FLinearColor(0.1f,0.2f,0.7f,0.0f));
		txt_Q->SetText(FText::FromName(skillName));
	}
	else if (skillTag == TagX)
	{
		img_X->SetColorAndOpacity(FLinearColor(0.1f,0.2f,0.7f,0.0f));
		txt_X->SetText(FText::FromName(skillName));
	}
}
