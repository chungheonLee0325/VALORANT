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


ABaseInteractor::ABaseInteractor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bNetLoadOnClient = true;
	SetReplicatingMovement(true);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(Mesh);
	
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("DetectWidget");
	DetectWidgetComponent->SetVisibility(false);
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
	DetectWidgetComponent->SetupAttachment(GetRootComponent());

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetSphereRadius(32.f);
	Sphere->SetCollisionProfileName(TEXT("Interactable"));
	Sphere->SetupAttachment(GetRootComponent());
}

void ABaseInteractor::OnRep_OwnerAgent()
{
	if (OwnerAgent)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, AgentName: %s"), __FUNCTION__, *OwnerAgent->GetName());
	}
	else
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OwnerAgent is nullptr"), __FUNCTION__);
	}
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
	if (nullptr != OwnerAgent || IsActorBeingDestroyed())
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
	UE_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	Agent->GetMesh()->GetChildrenComponents(true, ChildrenArray);
		
	// 자동으로 주워지는지 여부는 각자 CanAutoPickUp을 Override 해서 판단하도록 한다
	if (ServerOnly_CanAutoPickUp(Agent) == false)
	{
		return;
	}

	ServerRPC_Interact(Agent);
}

void ABaseInteractor::OnDetect(bool bIsDetect)
{
	UE_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
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
	if (HasAuthority())
	{
		Mesh->SetVisibility(bActive);
		Multicast_SetActive(bActive);
		//NET_LOG(LogTemp, Warning,TEXT("%s, 활성 상태: %d"), *GetActorNameOrLabel() ,bActive);
	}
	else
	{
		ServerRPC_SetActive(bActive);
	}
}

void ABaseInteractor::ServerRPC_PickUp_Implementation(ABaseAgent* Agent)
{
	if (nullptr == Agent)
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractorName: %s, Agent is nullptr"), __FUNCTION__, *GetName());
		return;
	}

	NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractorName: %s"), __FUNCTION__, *GetName());
	
	OwnerAgent = Agent;
	SetOwner(OwnerAgent);
	Multicast_PickUp(OwnerAgent);
}

void ABaseInteractor::Multicast_PickUp_Implementation(ABaseAgent* Agent)
{
	OnDetect(false);
	if (Agent->GetFindInteractorActor() == this)
	{
		Agent->ResetFindInteractorActor();
	}
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
	
	SetActive(true);
	
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

void ABaseInteractor::ServerRPC_Interact_Implementation(ABaseAgent* InteractAgent)
{
	//
}

void ABaseInteractor::ServerRPC_SetActive_Implementation(bool bActive)
{
	SetActive(bActive);
}

void ABaseInteractor::Multicast_SetActive_Implementation(bool bActive)
{
	Mesh->SetVisibility(bActive);
}