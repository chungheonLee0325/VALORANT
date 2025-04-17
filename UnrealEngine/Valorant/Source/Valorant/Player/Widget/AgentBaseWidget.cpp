// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentBaseWidget.h"

#include "GameplayTagContainer.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

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
