// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAbilitySystemComponent.h"


UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

