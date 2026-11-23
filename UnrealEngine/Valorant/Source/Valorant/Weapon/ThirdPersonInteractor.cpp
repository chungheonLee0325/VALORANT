// Fill out your copyright notice in the Description page of Project Settings.


#include "ThirdPersonInteractor.h"

#include "BaseWeaponAnim.h"
#include "GameManager/ValorantGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "ValorantObject/BaseInteractor.h"
#include "ValorantObject/Spike/Spike.h"
#include "BaseWeapon.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"

AThirdPersonInteractor::AThirdPersonInteractor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bNetLoadOnClient = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);
	
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"), false);
	Mesh->SetOwnerNoSee(true);
	Mesh->SetCastShadow(false);
}

void AThirdPersonInteractor::SetXraySetting() const
{
	const auto* ownerAgent = Cast<ABaseAgent>(GetOwner());
	if (!ownerAgent && IsValid(OwnerInteractor))
	{
		ownerAgent = OwnerInteractor->GetOwnerAgent();
	}
	if (ownerAgent)
	{
		if (const auto* MyPC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
		{
			if (const auto* MyPS = MyPC->GetPlayerState<AAgentPlayerState>())
			{
				if (const auto* OwnerPS = ownerAgent->GetPlayerState<AAgentPlayerState>())
				{
					const bool bSameTeam = MyPS != OwnerPS && MyPS->bIsBlueTeam == ownerAgent->IsBlueTeam();
					Mesh->SetRenderCustomDepth(bSameTeam);
				}
			}
		}
	}
}

void AThirdPersonInteractor::OnRep_Owner()
{
	Super::OnRep_Owner();
	SetXraySetting();
}

void AThirdPersonInteractor::MulticastRPC_InitSpike_Implementation(ASpike* Spike)
{
	if (!IsValid(Spike))
	{
		return;
	}
	OwnerInteractor = Spike;
	Mesh->SetSkeletalMeshAsset(Spike->GetMesh()->GetSkeletalMeshAsset());
	Mesh->SetRelativeScale3D(FVector(0.34f));
	SetXraySetting();
}

void AThirdPersonInteractor::MulticastRPC_InitWeapon_Implementation(ABaseWeapon* Weapon, const int WeaponId)
{
	auto* GameInstance = Cast<UValorantGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (nullptr == GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Called, GameInstance Is Null"), __FUNCTION__);
		return;
	}

	const auto& WeaponData = GameInstance->GetWeaponData(WeaponId);
	if (nullptr == WeaponData)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Called, WeaponData Load Fail (WeaponID : %d)"), __FUNCTION__, WeaponId);
		return;
	}
	
	auto* WeaponMeshAsset = WeaponData->WeaponMesh;
	if (nullptr == WeaponMeshAsset || nullptr == Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Called, WeaponMeshAsset Load Fail (WeaponID : %d)"), __FUNCTION__, WeaponId);
		return;
	}

	if (Weapon)
	{
		OwnerInteractor = Weapon;
	}

	// NET_LOG(LogTemp, Warning, TEXT("%hs Called, WeaponId is %d"), __FUNCTION__, WeaponId);
	Mesh->SetSkeletalMeshAsset(WeaponMeshAsset);
	Mesh->SetRelativeScale3D(FVector(0.34f));

	if (WeaponData->GunABPClass)
	{
		Mesh->SetAnimInstanceClass(WeaponData->GunABPClass);
	}
	
	SetXraySetting();
}
