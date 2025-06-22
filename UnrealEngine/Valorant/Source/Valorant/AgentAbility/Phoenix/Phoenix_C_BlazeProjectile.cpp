#include "Phoenix_C_BlazeProjectile.h"
#include "Phoenix_C_BlazeSplineWall.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

APhoenix_C_BlazeProjectile::APhoenix_C_BlazeProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    
    Sphere->SetSphereRadius(5.0f);
    Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sphere->SetVisibility(false);
    
    ProjectileMovement->InitialSpeed = StraightSpeed;
    ProjectileMovement->MaxSpeed = StraightSpeed;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
}

void APhoenix_C_BlazeProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(APhoenix_C_BlazeProjectile, MovementType);
    DOREPLIFETIME(APhoenix_C_BlazeProjectile, bIsActive);
}

void APhoenix_C_BlazeProjectile::BeginPlay()
{
    Super::BeginPlay();
    
    if (HasAuthority())
    {
        // 스플라인 벽 생성
        if (SplineWallClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.Instigator = GetInstigator();
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            FVector SpawnLocation = GetActorLocation();
            FRotator SpawnRotation = FRotator::ZeroRotator;
            
            SplineWall = GetWorld()->SpawnActor<APhoenix_C_BlazeSplineWall>(
                SplineWallClass, SpawnLocation, SpawnRotation, SpawnParams);
            
            if (SplineWall)
            {
                // 첫 번째 포인트 추가
                SplineWall->AddSplinePoint(GetActorLocation());
                
                // 주기적으로 스플라인 포인트 추가
                GetWorld()->GetTimerManager().SetTimer(SplineUpdateTimer, this, 
                    &APhoenix_C_BlazeProjectile::UpdateSplinePoint, SegmentSpawnInterval, true);
            }
        }
    }
}

void APhoenix_C_BlazeProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!bIsActive)
    {
        return;
    }
    
    if (HasAuthority())
    {
        if (MovementType == EBlazeMovementType::Curved)
        {
            ApplyCurveMovement(DeltaTime);
        }
        
        TotalDistanceTraveled += ProjectileMovement->Velocity.Size() * DeltaTime;
        
        if (TotalDistanceTraveled >= MaxWallLength)
        {
            StopProjectile();
        }
    }
}

void APhoenix_C_BlazeProjectile::SetMovementType(EBlazeMovementType Type)
{
    if (!HasAuthority())
    {
        return;
    }
    
    MovementType = Type;
    
    float NewSpeed = (Type == EBlazeMovementType::Straight) ? StraightSpeed : CurvedSpeed;
    ProjectileMovement->InitialSpeed = NewSpeed;
    ProjectileMovement->MaxSpeed = NewSpeed;
    ProjectileMovement->Velocity = GetActorForwardVector() * NewSpeed;
    
    Multicast_SetMovementType(Type);
}

void APhoenix_C_BlazeProjectile::Multicast_SetMovementType_Implementation(EBlazeMovementType Type)
{
    if (!HasAuthority())
    {
        MovementType = Type;
    }
}

void APhoenix_C_BlazeProjectile::UpdateSplinePoint()
{
    if (!HasAuthority() || !bIsActive || !SplineWall)
    {
        return;
    }
    
    FVector CurrentLocation = GetActorLocation();
    
    // 지면에 맞춰 위치 조정
    FHitResult HitResult;
    FVector TraceStart = CurrentLocation + FVector(0, 0, 100);
    FVector TraceEnd = CurrentLocation - FVector(0, 0, 500);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(GetOwner());
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, 
        ECollisionChannel::ECC_WorldStatic, QueryParams))
    {
        CurrentLocation = HitResult.Location;
    }
    
    // 스플라인 포인트 추가
    SplineWall->AddSplinePoint(CurrentLocation);
}

void APhoenix_C_BlazeProjectile::ApplyCurveMovement(float DeltaTime)
{
    if (!ProjectileMovement)
    {
        return;
    }
    
    CurrentCurveAngle += DeltaTime * CurveRate;
    CurrentCurveAngle = FMath::Clamp(CurrentCurveAngle, 0.0f, 90.0f);
    
    FVector CurrentVelocity = ProjectileMovement->Velocity;
    float CurrentSpeed = CurrentVelocity.Size();
    
    FQuat RotationQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(CurveRate * DeltaTime));
    FVector NewDirection = RotationQuat.RotateVector(CurrentVelocity.GetSafeNormal());
    
    ProjectileMovement->Velocity = NewDirection * CurrentSpeed;
    SetActorRotation(NewDirection.Rotation());
}

void APhoenix_C_BlazeProjectile::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    if (HasAuthority())
    {
        StopProjectile();
    }
}

void APhoenix_C_BlazeProjectile::StopProjectile()
{
    if (!HasAuthority() || !bIsActive)
    {
        return;
    }
    
    bIsActive = false;
    
    GetWorld()->GetTimerManager().ClearTimer(SplineUpdateTimer);
    ProjectileMovement->StopMovementImmediately();
    
    // 벽 완성
    if (SplineWall)
    {
        SplineWall->FinalizeWall();
    }
    
    // 투사체 제거
    SetLifeSpan(0.1f);
}