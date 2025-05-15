// Fill out your copyright notice in the Description page of Project Settings.


#include "KayoKnife.h"

#include "GameFramework/ProjectileMovementComponent.h"


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
	
	Mesh->SetRelativeScale3D(FVector(1.0f));
	Mesh->SetRelativeLocation(FVector(-60, 0, 0));
	Mesh->SetRelativeRotation(FRotator(-90, 0, 0));

	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
}

void AKayoKnife::BeginPlay()
{
	Super::BeginPlay();
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

void AKayoKnife::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);

	// 벽에 박히는 방향 계산: 벽의 Normal 반대 방향
	const FVector HitNormal = ImpactResult.ImpactNormal;
	const FVector KnifeForward = -HitNormal;

	// 회전 계산: 벡터 → Rotator
	const FRotator KnifeRotation = KnifeForward.Rotation();
	SetActorRotation(KnifeRotation);
	Mesh->SetRelativeRotation(FRotator(-90, 0, 0));

	// 벽 표면에 살짝 붙이기
	// SetActorLocation(ImpactResult.ImpactPoint); // 살짝 튀어나오게

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