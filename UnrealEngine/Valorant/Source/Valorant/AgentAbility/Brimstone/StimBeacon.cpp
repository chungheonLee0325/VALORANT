// Fill out your copyright notice in the Description page of Project Settings.


#include "StimBeacon.h"

#include "StimBeaconAnim.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"


AStimBeacon::AStimBeacon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	
	Sphere->SetSphereRadius(20.0f);
	Mesh->SetRelativeScale3D(FVector(1.0f));
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BeaconMeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Projectiles/Brimstone_Ability_SpeedStim/AB_Sarge_S0_E_Skelmesh.AB_Sarge_S0_E_Skelmesh'"));
	if (BeaconMeshAsset.Succeeded())
	{
		Mesh->SetSkeletalMeshAsset(BeaconMeshAsset.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Resource/Props/Projectiles/Brimstone_Ability_SpeedStim/ABP_StimBeacon.ABP_StimBeacon_C'"));
	if (AnimInstanceClass.Succeeded())
	{
		Mesh->SetAnimInstanceClass(AnimInstanceClass.Class);
	}
	
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
}

void AStimBeacon::BeginPlay()
{
	Super::BeginPlay();
	AnimInstance = Cast<UStimBeaconAnim>(Mesh->GetAnimInstance());
	AnimInstance->OnOutroEnded.AddDynamic(this, &AStimBeacon::OnOutroAnimationEnded);
	AnimInstance->OnDeployEnded.AddDynamic(this, &AStimBeacon::OnDeployAnimationEnded);
}

void AStimBeacon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ProjectileMovement->IsActive())
	{
		// 설치된 후 스르륵 설치되는 애니메이션 연출을 위해 전방 벡터가 날아가는 방향과 일치하도록 정렬
		const FVector ForwardDir = ProjectileMovement->Velocity.GetSafeNormal2D();
		const FRotator NewRot = ForwardDir.Rotation();
		SetActorRotation(FRotator(0, NewRot.Yaw, 0));
	}
}

void AStimBeacon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AStimBeacon, State);
}

void AStimBeacon::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);

	UE_LOG(LogTemp, Warning, TEXT("%hs Called, ImpactNormal: %s"), __FUNCTION__, *ImpactResult.ImpactNormal.ToString());
	if (ImpactResult.ImpactNormal.Z > 0.8f)
	{
		if (AnimInstance)
		{
			State = EStimBeaconState::ESBS_Active;
			AnimInstance->OnDeploy();
		}
		
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->SetActive(false);
	}
}

void AStimBeacon::OnOutroAnimationEnded()
{
	Destroy();
}

void AStimBeacon::OnDeployAnimationEnded()
{
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("%hs Called, Beacon is activated"), __FUNCTION__);
		// TODO: 범위 효과 시작
	}
}