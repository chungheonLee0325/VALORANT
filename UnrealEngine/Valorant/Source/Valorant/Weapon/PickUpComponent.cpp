// Fill out your copyright notice in the Description page of Project Settings.


#include "PickUpComponent.h"

#include "BaseWeapon.h"
#include "Player/Agent/BaseAgent.h"


// Sets default values
UPickUpComponent::UPickUpComponent()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void UPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	bIsEnableBeginOverlap = true;
	OnComponentBeginOverlap.AddDynamic(this, &UPickUpComponent::OnSphereBeginOverlap);
}

void UPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	if(ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		if (Agent->InteractionCapsule == Cast<UCapsuleComponent>(OtherComp))
		{
			return;
		}

		// 이미 무기를 들고있다면 무시한다.
		// TODO: 주무기, 보조무기, 근접무기 구분 필요
		TArray<USceneComponent*> ChildrenArray;
		Agent->GetMesh()->GetChildrenComponents(true, ChildrenArray);
		if (ChildrenArray.FindItemByClass<ABaseWeapon>())
		{
			return;
		}
		
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Agent);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);
		bIsEnableBeginOverlap = false;
	}
}

void UPickUpComponent::PickUp(ABaseAgent* Character)
{
}

void UPickUpComponent::SetEnableBeginOverlap()
{
	if (bIsEnableBeginOverlap)
	{
		return;
	}
	OnComponentBeginOverlap.AddDynamic(this, &UPickUpComponent::OnSphereBeginOverlap);
}
