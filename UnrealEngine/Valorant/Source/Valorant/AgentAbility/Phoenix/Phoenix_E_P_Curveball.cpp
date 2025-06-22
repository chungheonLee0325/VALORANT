// Fill out your copyright notice in the Description page of Project Settings.


#include "Phoenix_E_P_Curveball.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
APhoenix_E_P_Curveball::APhoenix_E_P_Curveball()
{
	PrimaryActorTick.bCanEverTick = true;

	// 메시 컴포넌트 생성
	CurveballMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CurveballMesh"));
	CurveballMesh->SetupAttachment(GetRootComponent());

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
	if (MeshAsset.Succeeded())
	{
		CurveballMesh->SetStaticMesh(MeshAsset.Object);
		CurveballMesh->SetWorldScale3D(FVector(0.3f));  // 크기 조정
	}

	// 충돌체 크기 조정
	if (Sphere)
	{
		Sphere->SetSphereRadius(15.0f);
	}

	// ProjectileMovement 설정 - 발로란트 실제 수치
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = InitialSpeed * 1.2f;  // 약간의 속도 증가 허용
		ProjectileMovement->ProjectileGravityScale = 0.0f;   // 중력 없음
		ProjectileMovement->bRotationFollowsVelocity = false; // 수동으로 회전 제어
		ProjectileMovement->bShouldBounce = true;
		ProjectileMovement->Bounciness = 0.5f;  // 벽에 부딪힐 때 반발력
		ProjectileMovement->Friction = 0.0f;
	}

	// 섬광 설정 - 발로란트 실제 수치
	SetLifeSpan(0.8f);  // 실제 폭발까지 시간
    
	// 피닉스 커브볼 섬광 시간 설정 
	MaxBlindDuration = 1.5f;   // 최대 1.1초 완전 실명
	MinBlindDuration = 1.1f;   // 최소 0.3초 완전 실명
	RecoveryDuration = 1.1f;   // 0.6초 회복
	DetonationDelay = 0.7f;    // 1.1초 후 폭발
	FlashRadius = 3300.0f;     // 33미터 반경
}

// Called when the game starts or when spawned
void APhoenix_E_P_Curveball::BeginPlay()
{
	Super::BeginPlay();
	
	// 초기 속도 설정
	if (ProjectileMovement)
	{
		FVector InitialVelocity = GetActorForwardVector() * InitialSpeed;
		ProjectileMovement->Velocity = InitialVelocity;
	}
}

// Called every frame
void APhoenix_E_P_Curveball::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ProjectileMovement || !ProjectileMovement->IsActive())
		return;

	TimeSinceSpawn += DeltaTime;

	// 회전 애니메이션
	if (CurveballMesh)
	{
		CurveballMesh->AddLocalRotation(SpinRate * DeltaTime);
	}

	// 곡선 궤적 적용
	if (TimeSinceSpawn >= CurveDelay && !bHasStartedCurving)
	{
		bHasStartedCurving = true;
	}

	if (bHasStartedCurving && TimeSinceSpawn <= MaxCurveTime)
	{
		// 현재 속도 방향
		FVector CurrentVelocity = ProjectileMovement->Velocity;
		float CurrentSpeed = CurrentVelocity.Size();
		FVector VelocityDirection = CurrentVelocity.GetSafeNormal();

		// 곡선 방향 계산 (오른쪽 또는 왼쪽)
		FVector RightVector = FVector::CrossProduct(VelocityDirection, FVector::UpVector).GetSafeNormal();
		if (bShouldCurveRight)
		{
			RightVector *= -1.0f;
		}

		// 곡선 강도 계산 - 처음엔 강하게, 나중엔 약하게
		float CurveProgress = (TimeSinceSpawn - CurveDelay) / (MaxCurveTime - CurveDelay);
		float CurveMultiplier = FMath::Lerp(1.0f, 0.3f, CurveProgress);
		float CurrentCurveStrength = CurveStrength * CurveMultiplier;

		// 새로운 속도 방향 계산
		FVector CurveForce = RightVector * CurrentCurveStrength * DeltaTime;
		FVector NewVelocity = CurrentVelocity + CurveForce;
        
		// 속도 유지하면서 방향만 변경
		NewVelocity = NewVelocity.GetSafeNormal() * CurrentSpeed;
		ProjectileMovement->Velocity = NewVelocity;

		// 투사체 회전을 속도 방향에 맞춤
		FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(NewVelocity.GetSafeNormal());
		SetActorRotation(NewRotation);
	}
}

void APhoenix_E_P_Curveball::SetCurveDirection(bool bCurveRight)
{
	bShouldCurveRight = bCurveRight;
}