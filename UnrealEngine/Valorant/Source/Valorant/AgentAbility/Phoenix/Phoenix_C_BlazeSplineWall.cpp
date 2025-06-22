#include "Phoenix_C_BlazeSplineWall.h"
#include "Components/SplineMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"

APhoenix_C_BlazeSplineWall::APhoenix_C_BlazeSplineWall()
{
	PrimaryActorTick.bCanEverTick = false;

	// 네트워크 설정
	bReplicates = true;
	SetReplicateMovement(true);

	// 스플라인 컴포넌트
	WallSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WallSpline"));
	SetRootComponent(WallSpline);

	// 기본 메시 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(
		TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (DefaultMesh.Succeeded())
	{
		WallMesh = DefaultMesh.Object;
	}

	CalculateTickValues();
}

void APhoenix_C_BlazeSplineWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void APhoenix_C_BlazeSplineWall::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 서버에서 지속 시간 타이머 설정
		GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this,
		                                       &APhoenix_C_BlazeSplineWall::OnElapsedDuration, WallDuration, false);
	}
}

void APhoenix_C_BlazeSplineWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void APhoenix_C_BlazeSplineWall::AddSplinePoint(const FVector& Location)
{
	if (!HasAuthority())
	{
		return;
	}

	// 서버에서 스플라인 포인트 추가
	WallSpline->AddSplinePoint(Location, ESplineCoordinateSpace::World);

	// 클라이언트에 동기화
	Multicast_AddSplinePoint(Location);
}

void APhoenix_C_BlazeSplineWall::Multicast_AddSplinePoint_Implementation(FVector Location)
{
	if (!HasAuthority())
	{
		WallSpline->AddSplinePoint(Location, ESplineCoordinateSpace::World);
	}

	// 실시간으로 스플라인 메시 업데이트
	UpdateSplineMesh();
}

void APhoenix_C_BlazeSplineWall::FinalizeWall()
{
	if (!HasAuthority())
	{
		return;
	}

	// 모든 클라이언트에서 벽 완성
	Multicast_FinalizeWall();
}

void APhoenix_C_BlazeSplineWall::Multicast_FinalizeWall_Implementation()
{
	// 최종 스플라인 메시 업데이트
	WallSpline->UpdateSpline();
	UpdateSplineMesh();

	// 서버에서만 충돌 이벤트 바인딩
	if (HasAuthority())
	{
		// 섬광 충돌 이벤트
		for (UBoxComponent* Collision : FlashCollisionComponents)
		{
			if (Collision)
			{
				Collision->OnComponentBeginOverlap.AddDynamic(this, &APhoenix_C_BlazeSplineWall::OnFlashBeginOverlap);
				Collision->OnComponentEndOverlap.AddDynamic(this, &APhoenix_C_BlazeSplineWall::OnFlashEndOverlap);
			}
		}

		// 데미지 충돌 이벤트
		for (UBoxComponent* Collision : DamageCollisionComponents)
		{
			if (Collision)
			{
				Collision->OnComponentBeginOverlap.AddDynamic(this, &APhoenix_C_BlazeSplineWall::OnDamageBeginOverlap);
				Collision->OnComponentEndOverlap.AddDynamic(this, &APhoenix_C_BlazeSplineWall::OnDamageEndOverlap);
			}
		}
	}
}

void APhoenix_C_BlazeSplineWall::UpdateSplineMesh()
{
	// 기존 스플라인 메시 제거
	for (USplineMeshComponent* SplineMesh : SplineMeshComponents)
	{
		if (SplineMesh)
		{
			SplineMesh->DestroyComponent();
		}
	}
	SplineMeshComponents.Empty();

	// 기존 충돌 컴포넌트 제거
	for (UBoxComponent* Collision : FlashCollisionComponents)
	{
		if (Collision)
		{
			Collision->DestroyComponent();
		}
	}
	FlashCollisionComponents.Empty();

	for (UBoxComponent* Collision : DamageCollisionComponents)
	{
		if (Collision)
		{
			Collision->DestroyComponent();
		}
	}
	DamageCollisionComponents.Empty();

	// 새로운 스플라인 메시 생성
	int32 NumPoints = WallSpline->GetNumberOfSplinePoints();
	if (NumPoints < 2)
	{
		return;
	}

	for (int32 i = 0; i < NumPoints - 1; i++)
	{
		// 스플라인 메시 컴포넌트 생성
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this,
		                                                                   USplineMeshComponent::StaticClass(),
		                                                                   FName(*FString::Printf(
			                                                                   TEXT("SplineMesh_%d"), i)));

		SplineMesh->SetStaticMesh(WallMesh);
		for (int index = 0; index < SplineMesh->GetMaterials().Num(); index++)
		{
			SplineMesh->SetMaterial(index, WallMaterial);
		}
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMesh->SetupAttachment(WallSpline);
		SplineMesh->RegisterComponent();

		// 스플라인 메시 설정
		FVector StartPos, StartTangent, EndPos, EndTangent;
		WallSpline->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::Local);
		WallSpline->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);

		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

		// 스케일 설정 (두께와 높이)
		SplineMesh->SetStartScale(FVector2D(WallThickness / 100.0f, WallHeight / 100.0f));
		SplineMesh->SetEndScale(FVector2D(WallThickness / 100.0f, WallHeight / 100.0f));

		SplineMeshComponents.Add(SplineMesh);

		SplineMesh->SetForwardAxis(ESplineMeshAxis::X);

		// 충돌 박스 생성 (서버에서만 실제 충돌 처리)
		if (HasAuthority())
		{
			FVector SegmentCenter = (StartPos + EndPos) / 2.0f;
			float SegmentLength = FVector::Dist(StartPos, EndPos);
			FVector Direction = (EndPos - StartPos).GetSafeNormal();
			FRotator Rotation = Direction.Rotation();

			// 1. 섬광 효과용 충돌 박스 (가까운 범위)
			UBoxComponent* FlashCollisionBox = NewObject<UBoxComponent>(this,
			                                                            UBoxComponent::StaticClass(),
			                                                            FName(*FString::Printf(
				                                                            TEXT("FlashCollision_%d"), i)));

			FlashCollisionBox->SetupAttachment(WallSpline);
			FlashCollisionBox->RegisterComponent();
			FlashCollisionBox->SetRelativeLocation(SegmentCenter);

			// 섬광 범위 설정
			float FlashThickness = WallThickness * FlashRangeMultiplier;
			FlashCollisionBox->SetBoxExtent(FVector(SegmentLength / 2.0f,
			                                        FlashThickness / 2.0f,
			                                        WallHeight * 5));
			FlashCollisionBox->SetRelativeRotation(Rotation);
			FlashCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			FlashCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
			FlashCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

			// 디버그 시각화 (개발 중에만)
#ifdef DEBUGTEST
			FlashCollisionBox->SetHiddenInGame(false);
			FlashCollisionBox->ShapeColor = FColor::Yellow;
#endif

			FlashCollisionComponents.Add(FlashCollisionBox);

			// 2. 데미지/힐 효과용 충돌 박스 (넓은 범위)
			UBoxComponent* DamageCollisionBox = NewObject<UBoxComponent>(this,
			                                                             UBoxComponent::StaticClass(),
			                                                             FName(*FString::Printf(
				                                                             TEXT("DamageCollision_%d"), i)));

			DamageCollisionBox->SetupAttachment(WallSpline);
			DamageCollisionBox->RegisterComponent();
			DamageCollisionBox->SetRelativeLocation(SegmentCenter);

			// 데미지 범위 설정
			float DamageThickness = WallThickness * DamageRangeMultiplier;
			DamageCollisionBox->SetBoxExtent(FVector(SegmentLength / 2.0f,
			                                         DamageThickness / 2.0f,
			                                         WallHeight * 5));
			DamageCollisionBox->SetRelativeRotation(Rotation);
			DamageCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			DamageCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
			DamageCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

			// 디버그 시각화 (개발 중에만)
#ifdef DEBUGTEST
			DamageCollisionBox->SetHiddenInGame(false);
			DamageCollisionBox->ShapeColor = FColor::Red;
#endif

			DamageCollisionComponents.Add(DamageCollisionBox);

			UE_LOG(LogTemp, Warning, TEXT("Blaze Collision %d - Flash: %fx%fx%f, Damage: %fx%fx%f"),
			       i,
			       SegmentLength, FlashThickness, WallHeight,
			       SegmentLength, DamageThickness, WallHeight);
		}
	}
}

void APhoenix_C_BlazeSplineWall::OnFlashBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                     bool bFromSweep,
                                                     const FHitResult& SweepResult)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		if (!FlashOverlappedAgents.Contains(Agent))
		{
			FlashOverlappedAgents.Add(Agent);

			// 첫 번째 에이전트가 들어왔을 때 타이머 시작
			if (FlashOverlappedAgents.Num() == 1 && !GetWorld()->GetTimerManager().IsTimerActive(FlashTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(FlashTimerHandle, this,
				                                       &APhoenix_C_BlazeSplineWall::ApplyFlashEffect,
				                                       EffectApplicationInterval, true);

				// 즉시 첫 효과 적용
				Agent->AdjustFlashEffectDirect(0.25f, 0.25f);
			}
		}
	}
}

void APhoenix_C_BlazeSplineWall::OnFlashEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !OtherActor)
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		FlashOverlappedAgents.Remove(Agent);

		if (FlashOverlappedAgents.Num() == 0)
		{
			GetWorld()->GetTimerManager().ClearTimer(FlashTimerHandle);
		}
	}
}

void APhoenix_C_BlazeSplineWall::OnDamageBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                      bool bFromSweep,
                                                      const FHitResult& SweepResult)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		if (!DamageOverlappedAgents.Contains(Agent))
		{
			DamageOverlappedAgents.Add(Agent);

			// 첫 번째 에이전트가 들어왔을 때 타이머 시작
			if (DamageOverlappedAgents.Num() == 1 && !GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this,
				                                       &APhoenix_C_BlazeSplineWall::ApplyDamageEffect,
				                                       EffectApplicationInterval, true);

				// 즉시 첫 효과 적용
				ApplyDamageEffect();
			}
		}
	}
}

void APhoenix_C_BlazeSplineWall::OnDamageEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !OtherActor)
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		DamageOverlappedAgents.Remove(Agent);

		if (DamageOverlappedAgents.Num() == 0)
		{
			GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		}
	}
}

void APhoenix_C_BlazeSplineWall::ApplyDamageEffect()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
	{
		return;
	}

	TArray<ABaseAgent*> AgentsCopy = DamageOverlappedAgents.Array();

	for (ABaseAgent* Agent : AgentsCopy)
	{
		if (!IsValid(Agent))
		{
			DamageOverlappedAgents.Remove(Agent);
			continue;
		}

		// Phoenix 자신인지 확인
		bool bIsSelf = IsPhoenixSelf(Agent);

		if (bIsSelf)
		{
			// Phoenix 자신에게는 힐 적용
			if (GameplayEffect)
			{
				Agent->ServerApplyHealthGE(GameplayEffect, HealPerTick);
			}
		}
		else
		{
			// 모든 다른 에이전트(아군 포함)에게는 데미지 적용
			if (GameplayEffect)
			{
				Agent->ServerApplyHealthGE(GameplayEffect, -DamagePerTick);
			}
		}
	}
}

void APhoenix_C_BlazeSplineWall::ApplyFlashEffect()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
	{
		return;
	}
	// 섬광 효과 적용
	for (ABaseAgent* Agent : FlashOverlappedAgents.Array())
	{
		Agent->AdjustFlashEffectDirect(0.25f, 0.25f);
	}
}

bool APhoenix_C_BlazeSplineWall::IsPhoenixSelf(AActor* Actor) const
{
	// Instigator와 같은 액터인지 확인
	return Actor == GetInstigator();
}

void APhoenix_C_BlazeSplineWall::OnElapsedDuration()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);

	SetLifeSpan(0.1f);
}

void APhoenix_C_BlazeSplineWall::CalculateTickValues()
{
	DamagePerTick = DamagePerSecond * EffectApplicationInterval;
	HealPerTick = HealPerSecond * EffectApplicationInterval;
}
