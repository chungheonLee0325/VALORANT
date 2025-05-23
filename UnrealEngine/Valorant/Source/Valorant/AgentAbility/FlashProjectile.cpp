#include "FlashProjectile.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/Component/FlashComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AFlashProjectile::AFlashProjectile()
{
    // BaseProjectile 기본 설정 조정
    ProjectileMovement->InitialSpeed = 1000.0f;
    ProjectileMovement->MaxSpeed = 1000.0f;
    ProjectileMovement->ProjectileGravityScale = 1.5f;
    ProjectileMovement->Bounciness = 0.3f;
    
    // 3초 후 자동 폭발
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
    
    // 이미 폭발 예약되었으면 무시
    if (bIsScheduledToExplode)
        return;
    
    // 폭발 예약
    bIsScheduledToExplode = true;
    
    // 약간의 지연 후 폭발 (발로란트 스타일)
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AFlashProjectile::ExplodeFlash, DetonationDelay, false);
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

    // 폭발 사운드 재생
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
    }

    // 주변 플레이어들에게 섬광 효과 적용
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (ABaseAgent* Agent = Cast<ABaseAgent>(Actor))
        {
            float FlashIntensity = 0.0f;
            if (IsPlayerInFlashRange(Agent, FlashIntensity) && HasLineOfSight(Agent))
            {
                ApplyFlashEffectToPlayer(Agent, FlashIntensity);
            }
        }
    }

    // 디버그 구체 그리기
    if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
    {
        DrawDebugSphere(GetWorld(), GetActorLocation(), FlashRadius, 16, FColor::Yellow, false, 3.0f);
    }

    // 투사체 제거
    Destroy();
}

bool AFlashProjectile::IsPlayerInFlashRange(ABaseAgent* Player, float& OutIntensity)
{
    if (!Player || Player->IsDead())
        return false;

    float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
    
    if (Distance <= FlashRadius)
    {
        // 거리에 따른 강도 계산 (가까울수록 강함)
        float DistanceRatio = 1.0f - (Distance / FlashRadius);
        OutIntensity = FMath::Clamp(DistanceRatio * MaxFlashIntensity, 0.0f, MaxFlashIntensity);
        return true;
    }

    OutIntensity = 0.0f;
    return false;
}

bool AFlashProjectile::HasLineOfSight(ABaseAgent* Player)
{
    if (!Player)
        return false;

    // 레이캐스트로 시야 차단 체크
    FHitResult HitResult;
    FVector Start = GetActorLocation();
    FVector End = Player->GetActorLocation();

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(Player);

    // 벽이 있으면 섬광 효과 감소
    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams);

    bHit = false;

    return !bHit; // 벽이 없으면 true
}

void AFlashProjectile::ApplyFlashEffectToPlayer(ABaseAgent* Player, float Intensity)
{
    if (!Player || !HasAuthority())
        return;

    // FlashComponent를 통해 시각 효과 적용
    if (UFlashComponent* FlashComp = Player->FindComponentByClass<UFlashComponent>())
    {
        FlashComp->StartFlashEffect(Intensity, FlashDuration);
    }

    // GameplayEffect를 통해 게임플레이 효과 적용 (움직임 저해 등)
    if (FlashGameplayEffect && Player->GetASC())
    {
        Player->ServerApplyGE(FlashGameplayEffect);
    }
}