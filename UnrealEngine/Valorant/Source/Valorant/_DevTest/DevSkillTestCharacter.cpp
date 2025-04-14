// Fill out your copyright notice in the Description page of Project Settings.


#include "DevSkillTestCharacter.h"

#include "AbilitySystemComponent.h"


// Sets default values
ADevSkillTestCharacter::ADevSkillTestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void ADevSkillTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this,this);
		// AbilitySystemComponent->SetNumericAttributeBase(U뭐시기, Init뭐시기);
	}
}

UAbilitySystemComponent* ADevSkillTestCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called every frame
void ADevSkillTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ADevSkillTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

