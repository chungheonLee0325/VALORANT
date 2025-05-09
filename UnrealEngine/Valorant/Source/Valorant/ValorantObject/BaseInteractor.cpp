// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseInteractor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/Agent/BaseAgent.h"


ABaseInteractor::ABaseInteractor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(Mesh);
	
	InteractWidget = CreateDefaultSubobject<UWidgetComponent>("InteractWidget");
	InteractWidget->SetVisibility(false);
	InteractWidget->SetupAttachment(GetRootComponent());

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetSphereRadius(32.f);
	Sphere->SetCollisionProfileName(TEXT("Interactable"));
	Sphere->SetupAttachment(GetRootComponent());
}

void ABaseInteractor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseInteractor::ServerOnly_OnSphereBeginOverlap);
	}
}

void ABaseInteractor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseInteractor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseInteractor, OwnerAgent);
}

void ABaseInteractor::ServerOnly_OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (nullptr != OwnerAgent)
	{
		return;
	}

	ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor);
	if (nullptr == Agent)
	{
		return;
	}
	
	if (Agent->InteractionCapsule == Cast<UCapsuleComponent>(OtherComp))
	{
		return;
	}

	TArray<USceneComponent*> ChildrenArray;
	Agent->GetMesh()->GetChildrenComponents(true, ChildrenArray);
		
	// 자동으로 주워지는지 여부는 각자 CanAutoPickUp을 Override 해서 판단하도록 한다
	if (ServerOnly_CanAutoPickUp(Agent) == false)
	{
		return;
	}

	ServerRPC_PickUp(Agent);
}

void ABaseInteractor::InteractActive(bool bIsActive)
{
	InteractWidget->SetVisibility(bIsActive);
}

bool ABaseInteractor::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	return true;
}

bool ABaseInteractor::ServerOnly_CanDrop() const
{
	return true;
}

void ABaseInteractor::ServerRPC_PickUp_Implementation(ABaseAgent* Agent)
{
	OwnerAgent = Agent;
	SetOwner(OwnerAgent);
}

void ABaseInteractor::ServerRPC_Drop_Implementation()
{
	if (nullptr == OwnerAgent)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("드롭"));
	
	// TODO: 툭 놓는게 아니라 던지도록 변경
	FDetachmentTransformRules DetachmentRule(
		EDetachmentRule::KeepWorld,
		EDetachmentRule::KeepWorld,
		EDetachmentRule::KeepRelative,
		true
	);
	DetachFromActor(DetachmentRule);
	const FVector& ForwardVector = OwnerAgent->GetActorForwardVector();
	const FVector& FeetLocation = OwnerAgent->GetMovementComponent()->GetActorFeetLocation();
	const FVector Offset = FVector(0, 0, 32);
	SetActorLocation(FeetLocation + Offset + ForwardVector * 300);
	
	OwnerAgent = nullptr;
	SetOwner(nullptr);
}