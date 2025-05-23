#include "FlashProjectile.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/Component/FlashComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

AFlashProjectile::AFlashProjectile()
{
    ProjectileMovement->InitialSpeed = 1000.0f;
    ProjectileMovement->MaxSpeed = 1000.0f;
    ProjectileMovement->ProjectileGravityScale = 1.5f;
    ProjectileMovement->Bounciness = 0.3f;
    
    SetLifeSpan(3.0f);
}

void AFlashProjectile::BeginPlay()
{
    Super::BeginPlay();
    
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AFlashProjectile::ExplodeFlash, DetonationDelay, false);
}

void AFlashProjectile::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
    
    // if (bIsScheduledToExplode)
    //     return;
    //
    // bIsScheduledToExplode = true;
    //
    // FTimerHandle TimerHandle;
    // GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AFlashProjectile::ExplodeFlash, DetonationDelay, false);
}

void AFlashProjectile::ExplodeFlash()
{
    if (!HasAuthority())
        return;

    // 폭발 이펙트 스폰
    if (ExplosionEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
    }

    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
    }

    // 범위 내 플레이어들 확인
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (ABaseAgent* Agent = Cast<ABaseAgent>(Actor))
        {
            if (Agent->IsDead())
                continue;

            float BlindDuration = 0.0f;
            
            // 거리 기반으로 완전 실명 시간 계산
            if (IsPlayerInFlashRange(Agent, BlindDuration) && HasLineOfSight(Agent))
            {
                // 각 클라이언트에서 시야 각도 체크 후 적용
                MulticastApplyFlashEffect(BlindDuration);
            }
        }
    }

    // 디버그 표시
    if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
    {
        DrawDebugSphere(GetWorld(), GetActorLocation(), FlashRadius, 16, FColor::Yellow, false, 5.0f);
    }

    Destroy();
}

bool AFlashProjectile::IsPlayerInFlashRange(ABaseAgent* Player, float& OutBlindDuration)
{
    if (!Player)
        return false;

    float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
    
    if (Distance <= FlashRadius)
    {
        // 거리 비율 계산 (가까울수록 1.0에 가까움)
        float DistanceRatio = 1.0f - (Distance / FlashRadius);
        
        // 거리에 따라 완전 실명 시간만 변경
        OutBlindDuration = FMath::Lerp(MinBlindDuration, MaxBlindDuration, DistanceRatio);
        
        return true;
    }

    OutBlindDuration = 0.0f;
    return false;
}

bool AFlashProjectile::HasLineOfSight(ABaseAgent* Player)
{
    if (!Player)
        return false;

    FHitResult HitResult;
    FVector Start = GetActorLocation();
    FVector End = Player->GetActorLocation();

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(Player);

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams);
    
    return !bHit;
}

void AFlashProjectile::MulticastApplyFlashEffect_Implementation(float BlindDuration)
{
    // 모든 클라이언트에서 실행
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (ABaseAgent* Agent = Cast<ABaseAgent>(Actor))
        {
            if (Agent->IsDead())
                continue;

            // 클라이언트별로 시야 각도 체크 후 적용
            if (UFlashComponent* FlashComp = Agent->FindComponentByClass<UFlashComponent>())
            {
                FlashComp->CheckViewAngleAndApplyFlash(GetActorLocation(), BlindDuration, RecoveryDuration);
            }
        }
    }
}
