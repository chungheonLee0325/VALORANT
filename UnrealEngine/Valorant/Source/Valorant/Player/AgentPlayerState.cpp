// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerState.h"

#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Valorant/Agent/BaseAgent.h"
#include "Valorant/AbilitySystem/Attributes/BaseAttributeSet.h"

AAgentPlayerState::AAgentPlayerState()
{
	ASC = CreateDefaultSubobject<UAgentAbilitySystemComponent>(TEXT("ASC"));
	ASC-> SetIsReplicated(true);

	// GE에 의해 값이 변경될 때, GE 인스턴스를 복제할 범위 설정
	// Full - 항상 복제 / Minimal - GE에 의해 변경되는 AttributeData 값만 복제 / Mixed - 혼합
	// AttributeData에 ReplicatedUsing 설정을 하는 것과는 별개의 개념.
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
	
	NetUpdateFrequency = 100.f;
	MinNetUpdateFrequency = 33.f;
}

void AAgentPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && ASC) {
		//TODO: 레거시. 후에 Initial목적 GE 따로 제작
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), 100.f);
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), 100.f);
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(), 0.f);
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), 1000.f);
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), 600.f);
		
		UE_LOG(LogTemp, Warning, TEXT("Health: %f"), ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute()));
		UE_LOG(LogTemp, Warning, TEXT("Armor: %f"), ASC->GetNumericAttribute(UBaseAttributeSet::GetArmorAttribute()));
		UE_LOG(LogTemp, Warning, TEXT("WalkSpeed: %f"), ASC->GetNumericAttribute(UBaseAttributeSet::GetMoveSpeedAttribute()));
	}
}

UAbilitySystemComponent* AAgentPlayerState::GetAbilitySystemComponent() const
{
	return ASC;
}

UBaseAttributeSet* AAgentPlayerState::GetBaseAttributeSet() const
{
	return BaseAttributeSet;
}

UAttributeSet* AAgentPlayerState::GetAttributeSet() const
{
	return BaseAttributeSet;
}

float AAgentPlayerState::GetHealth() const
{
	return BaseAttributeSet->GetHealth();
}

float AAgentPlayerState::GetMaxHealth() const
{
	return BaseAttributeSet->GetMaxHealth();
}

float AAgentPlayerState::GetArmor() const
{
	return BaseAttributeSet->GetArmor();
}

float AAgentPlayerState::GetMoveSpeed() const
{
	return BaseAttributeSet->GetMoveSpeed();
}



