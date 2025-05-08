// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValorantPickUpComponent.h"

#include "ValorantWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Player/Agent/BaseAgent.h"

UValorantPickUpComponent::UValorantPickUpComponent()
{
	// Setup the Sphere Collision
	SphereRadius = 32.f;
}

void UValorantPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	OnComponentBeginOverlap.AddDynamic(this, &UValorantPickUpComponent::OnSphereBeginOverlap);
}

void UValorantPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	if(ABaseAgent* Character = Cast<ABaseAgent>(OtherActor))
	{
		if (Character->InteractionCapsule == Cast<UCapsuleComponent>(OtherComp))
		{
			return;
		}

		TArray<USceneComponent*> ChildrenArray;
		Character->GetMesh()->GetChildrenComponents(true, ChildrenArray);
		
		// TODO: 주무기, 보조무기, 근접무기 구분 필요
		if (ChildrenArray.FindItemByClass<UValorantWeaponComponent>())
		{
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("??"));
		for (auto Element : Character->GetInstanceComponents())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Element->GetName());
		}
		
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Character);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);
	}
}

void UValorantPickUpComponent::PickUp(ABaseAgent* Character)
{
	if (Character != nullptr)
	{
		OnPickUp.Broadcast(Character);
		OnComponentBeginOverlap.RemoveAll(this);
	}
}

void UValorantPickUpComponent::SetEnableBeginOverlap()
{
	OnComponentBeginOverlap.AddDynamic(this, &UValorantPickUpComponent::OnSphereBeginOverlap);
}