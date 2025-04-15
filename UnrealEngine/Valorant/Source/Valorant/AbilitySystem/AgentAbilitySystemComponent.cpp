// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"

#include "Attributes/BaseAttributeSet.h"


UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAgentAbilitySystemComponent::InitializeData(int32 agentID)
{
	UDataTable* AgentDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/BluePrint/DataTable/dt_Agent.dt_Agent"));
	
	if (!AgentDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("데이터 테이블을 로드할 수 없습니다"));
		return;
	}

	FString rowString = FString::FromInt(agentID);
	FName rowName(*rowString);
	
	FAgentData* agentData = AgentDataTable->FindRow<FAgentData>(rowName, TEXT(""));
	if (!agentData)
	{
		UE_LOG(LogTemp, Error, TEXT("해당 ID의 agent를 찾을 수 없어요. iD: %d"), agentID);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Agent Name: %s"), *agentData->AgentName);
	AgentData = agentData;

	SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), AgentData->BaseHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), AgentData->MaxHealth);
	SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(),AgentData->BaseArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), AgentData->MaxArmor);
	SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), AgentData->Speed);
}

void UAgentAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAgentAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

