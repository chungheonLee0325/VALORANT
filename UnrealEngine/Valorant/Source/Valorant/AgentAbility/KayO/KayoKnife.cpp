// Fill out your copyright notice in the Description page of Project Settings.


#include "KayoKnife.h"

#include "KayoKnifeAnim.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AKayoKnife::AKayoKnife()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Projectiles/KAYO_Ability_Knife/AB_Grenadier_S0_E_Knife_Skelmesh.AB_Grenadier_S0_E_Knife_Skelmesh'"));
	if (MeshAsset.Succeeded())
	{
		Mesh->SetSkeletalMesh(MeshAsset.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Resource/Props/Projectiles/KAYO_Ability_Knife/ABP_Knife.ABP_Knife_C'"));
	if (AnimInstanceClass.Succeeded())
	{
		Mesh->SetAnimInstanceClass(AnimInstanceClass.Class);
	}

	Sphere->SetSphereRadius(5.0f);
	Mesh->SetRelativeScale3D(FVector(1.0f));
	Mesh->SetRelativeLocation(FVector(-50, 0, 0));
	Mesh->SetRelativeRotation(FRotator(-90, 0, 0));

	ProjectileMovement->bAutoActivate = bAutoActivate;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
}

void AKayoKnife::BeginPlay()
{
	Super::BeginPlay();
	AnimInstance = Cast<UKayoKnifeAnim>(Mesh->GetAnimInstance());
}

void AKayoKnife::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ProjectileMovement->IsActive())
	{
		const FRotator RotationPerTick(0, 1080.f * DeltaSeconds, 0);
		Mesh->AddLocalRotation(RotationPerTick);
	}
}

void AKayoKnife::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKayoKnife, State);
}

void AKayoKnife::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	
	const FVector HitNormal = ImpactResult.ImpactNormal;
	const FVector KnifeForward = -HitNormal;
	
	const FRotator KnifeRotation = KnifeForward.Rotation();
	SetActorRotation(KnifeRotation);
	Mesh->SetRelativeRotation(FRotator(-90, 0, 0));

	State = EKnifeState::EKS_Active;
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetActive(false);
	
	if (GetWorld()->GetTimerManager().IsTimerActive(ActiveTimerHandle) == false)
	{
		GetWorld()->GetTimerManager().SetTimer(ActiveTimerHandle, this, &AKayoKnife::ActiveSuppressionZone, ActiveTime, false);
	}
}

void AKayoKnife::ActiveSuppressionZone()
{
	Destroy();
}

void AKayoKnife::OnEquip() const
{
	if (AnimInstance)
	{
		AnimInstance->OnKnifeEquip();
	}
}

void AKayoKnife::OnThrow()
{
	State = EKnifeState::EKS_Throw;
	ProjectileMovement->SetActive(true);
}

void AKayoKnife::SetIsThirdPerson(const bool bNew)
{
	bIsThirdPerson = bNew;
}
