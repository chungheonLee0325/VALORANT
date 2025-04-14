// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AgentAbilitySystemComponent.generated.h"


UCLASS()
class VALORANT_API UAgentAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UAgentAbilitySystemComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
