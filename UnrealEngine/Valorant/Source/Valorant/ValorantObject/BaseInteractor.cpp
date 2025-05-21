// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseInteractor.h"

#include "Valorant.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Net/UnrealNetwork.h"
#include "Player/Agent/BaseAgent.h"
#include "Weapon/ThirdPersonInteractor.h"


ABaseInteractor::ABaseInteractor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bNetLoadOnClient = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	// Mesh->SetOnlyOwnerSee(true);
	SetRootComponent(Mesh);
	
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("DetectWidget");
	DetectWidgetComponent->SetVisibility(false);
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
	DetectWidgetComponent->SetupAttachment(GetRootComponent());

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetVisibility(false);
	Sphere->SetSphereRadius(32.f);
	Sphere->SetCollisionProfileName(TEXT("Interactable"));
	Sphere->SetupAttachment(GetRootComponent());
}

void ABaseInteractor::OnRep_OwnerAgent()
{
	if (OwnerAgent)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractorName: %s, AgentName: %s"), __FUNCTION__, *GetName(), *OwnerAgent->GetName());
		SetOwner(OwnerAgent);
		OnDetect(false);
		Mesh->SetOnlyOwnerSee(true);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		auto* Agent = Cast<ABaseAgent>(GetWorld()->GetFirstPlayerController()->GetCharacter());
		if (Agent && Agent->GetFindInteractorActor() == this)
		{
			Agent->ResetFindInteractorActor();
		}
	}
	else
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OwnerAgent is nullptr"), __FUNCTION__);
		SetOwner(nullptr);
		OnDetect(false);
		Mesh->SetOnlyOwnerSee(false);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (ThirdPersonInteractor)
		{
			ThirdPersonInteractor->Destroy();
		}
	}
}

void ABaseInteractor::SetOwnerAgent(ABaseAgent* NewAgent)
{
	OwnerAgent = NewAgent;
	OnRep_OwnerAgent();
}

void ABaseInteractor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseInteractor::ServerOnly_OnSphereBeginOverlap);
	}
}

void ABaseInteractor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseInteractor::BeginDestroy()
{
	Super::BeginDestroy();
	if (ThirdPersonInteractor)
	{
		ThirdPersonInteractor->Destroy();
	}
}

void ABaseInteractor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseInteractor, OwnerAgent);
	DOREPLIFETIME(ABaseInteractor, ThirdPersonInteractor);
}

void ABaseInteractor::ServerOnly_OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (nullptr != OwnerAgent || IsActorBeingDestroyed())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OwnerAgent is not nullptr"), __FUNCTION__);
		return;
	}

	ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor);
	if (nullptr == Agent)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OtherActor is not Agent or nullptr"), __FUNCTION__);
		return;
	}
	
	if (Agent->InteractionCapsule == Cast<UCapsuleComponent>(OtherComp))
	{
		return;
	}

	TArray<USceneComponent*> ChildrenArray;
	// UE_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	Agent->GetMesh()->GetChildrenComponents(true, ChildrenArray);
		
	// 자동으로 주워지는지 여부는 각자 CanAutoPickUp을 Override 해서 판단하도록 한다
	if (ServerOnly_CanAutoPickUp(Agent) == false)
	{
		return;
	}

	NET_LOG(LogTemp, Warning, TEXT("%hs Called, Interactor Name is %s, Interact with %s"), __FUNCTION__, *GetName(), *Agent->GetName());
	ServerRPC_Interact(Agent);
}

void ABaseInteractor::OnDetect(bool bIsDetect)
{
	// NET_LOG(LogTemp, Warning, TEXT("%hs Called, %hs"), __FUNCTION__, bIsDetect ? "True" : "False");
	DetectWidgetComponent->SetVisibility(bIsDetect);
	if (bIsDetect)
	{
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(1);
	}
	else
	{
		Mesh->SetRenderCustomDepth(false);
		Mesh->SetCustomDepthStencilValue(0);
	}
}

bool ABaseInteractor::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	return true;
}

bool ABaseInteractor::ServerOnly_CanDrop() const
{
	return true;
}

bool ABaseInteractor::ServerOnly_CanInteract() const
{
	return true;
}

void ABaseInteractor::SetActive(bool bActive)
{
	Mesh->SetHiddenInGame(!bActive, true);
	if (ThirdPersonInteractor)
	{
		ThirdPersonInteractor->SetActorHiddenInGame(!bActive);
	}
	Sphere->SetVisibility(false);
	ServerRPC_SetActive(bActive);
}


void ABaseInteractor::ServerRPC_PickUp_Implementation(ABaseAgent* Agent)
{
	if (nullptr == Agent)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, InteractorName: %s, Agent is nullptr"), __FUNCTION__, *GetName());
		return;
	}
	
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractorName: %s"), __FUNCTION__, *GetName());
	SetOwnerAgent(Agent);
	MulticastRPC_BroadcastOnPickUp();
}

void ABaseInteractor::ServerRPC_Drop_Implementation()
{
	if (nullptr == OwnerAgent)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractorName: %s, OwnerAgent is nullptr"), __FUNCTION__, *GetName());
		return;
	}

	NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractorName: %s"), __FUNCTION__, *GetName());
	
	if (OwnerAgent->GetCurrentInterator() == this)
	{
		OwnerAgent->ServerRPC_SetCurrentInteractor(nullptr);
	}

	MulticastRPC_BroadcastOnDrop();
	Multicast_SetActive(true);
	
	// TODO: 툭 놓는게 아니라 던지도록 변경
	FDetachmentTransformRules DetachmentRule(
		EDetachmentRule::KeepWorld,
		EDetachmentRule::KeepWorld,
		EDetachmentRule::KeepRelative,
		true
	);
	
	Mesh->DetachFromComponent(DetachmentRule);
	const FVector& ForwardVector = OwnerAgent->GetActorForwardVector();
	const FVector& FeetLocation = OwnerAgent->GetMovementComponent()->GetActorFeetLocation();
	const FVector Offset = FVector(0, 0, 32);
	const FVector NewLocation = FeetLocation + Offset + ForwardVector * 300;
	SetActorLocation(NewLocation);
	SetOwnerAgent(nullptr);
}

void ABaseInteractor::ServerRPC_Interact_Implementation(ABaseAgent* InteractAgent)
{
	//
	if (InteractAgent == nullptr) NET_LOG(LogTemp, Error, TEXT("%hs Called, InteractAgent is nullptr"), __FUNCTION__);
}

void ABaseInteractor::ServerRPC_SetActive_Implementation(bool bActive)
{
	Multicast_SetActive(bActive);
}

void ABaseInteractor::Multicast_SetActive_Implementation(bool bActive)
{
	Mesh->SetHiddenInGame(!bActive, true);
	if (ThirdPersonInteractor)
	{
		ThirdPersonInteractor->SetActorHiddenInGame(!bActive);
	}
	Sphere->SetVisibility(false);
}

void ABaseInteractor::MulticastRPC_BroadcastOnDrop_Implementation()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnInteractorDrop.Broadcast();
}

void ABaseInteractor::MulticastRPC_BroadcastOnPickUp_Implementation()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnPickUp.Broadcast();
}

void ABaseInteractor::PlayEquipAnimation()
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	OnEquip.Broadcast();
	if (OwnerAgent)
	{
		OwnerAgent->OnEquip();
	}
}