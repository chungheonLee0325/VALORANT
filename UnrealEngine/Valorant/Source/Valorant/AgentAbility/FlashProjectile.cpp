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
}

void AFlashProjectile::ExplodeFlash()
{
    if (!HasAuthority())
        return;

    // 섬광이 터진 정확한 위치 저장
    FVector FlashLocation = GetActorLocation();

    NetMulti_ExplosionEffects(FlashLocation);

    // 범위 내 플레이어들 확인
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), FoundActors);

    bool bHasValidTargets = false;
    float MaxBlindDurationFound = 0.0f;

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
                bHasValidTargets = true;
                MaxBlindDurationFound = FMath::Max(MaxBlindDurationFound, BlindDuration);
            }
        }
    }
    
    // 섬광 위치와 타입 정보를 함께 전달
    if (bHasValidTargets)
    {
        MulticastApplyFlashEffect(MaxBlindDurationFound, FlashLocation, FlashType);
    }

    // 디버그 표시
    if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
    {
        //DrawDebugSphere(GetWorld(), FlashLocation, FlashRadius, 16, FColor::Yellow, false, 5.0f);
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

void AFlashProjectile::MulticastApplyFlashEffect_Implementation(float BlindDuration, FVector FlashLocation, EFlashType InFlashType)
{
    // 섬광 위치가 유효한지 확인
    if (FlashLocation.IsZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("경고: 섬광 위치가 Zero입니다. GetActorLocation 사용"));
        FlashLocation = GetActorLocation();
    }

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (APlayerController* PC = Iterator->Get())
        {
            if (ABaseAgent* LocalPlayer = Cast<ABaseAgent>(PC->GetPawn()))
            {
                // 로컬 플레이어가 아니면 건너뛰기
                if (!LocalPlayer->IsLocallyControlled() || LocalPlayer->IsDead())
                    continue;

                float LocalBlindDuration = 0.0f;
                if (!IsPlayerInFlashRange(LocalPlayer, LocalBlindDuration) || !HasLineOfSight(LocalPlayer))
                    continue;

                // 로컬 플레이어에 대해서만 섬광 적용 - 위치와 타입 정보 전달
                if (UFlashComponent* FlashComp = LocalPlayer->FindComponentByClass<UFlashComponent>())
                {
                    // 정확한 섬광 위치 전달
                    FlashComp->CheckViewAngleAndApplyFlash(FlashLocation, LocalBlindDuration, RecoveryDuration, InFlashType);
                    
                    UE_LOG(LogTemp, Warning, TEXT("섬광 적용: 위치=%s, 지속시간=%.2f, 타입=%d"), 
                        *FlashLocation.ToString(), LocalBlindDuration, (int32)InFlashType);
                }
                
                // 로컬 플레이어를 찾았으므로 루프 종료
                break;
            }
        }
    }


    // 섬광에 걸린 모든 대상에게 VFX 적용 (서버와 클라이언트 모두에서)
    if (FlashedTargetVFX)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), FoundActors);
    
        for (AActor* Actor : FoundActors)
        {
            if (ABaseAgent* Agent = Cast<ABaseAgent>(Actor))
            {
                if (Agent->IsDead())
                    continue;
    
                BlindDuration = 0.0f;
                if (IsPlayerInFlashRange(Agent, BlindDuration) && HasLineOfSight(Agent))
                {
                    // 1인칭 시점에서는 자신의 VFX가 안보이도록
                    if (!Agent->IsLocallyControlled())
                    {
                        // 실제 적용된 지속시간 계산 (시야각 고려)
                        if (UFlashComponent* FlashComp = Agent->FindComponentByClass<UFlashComponent>())
                        {
                            float ViewAngleMultiplier = FlashComp->CalculateViewAngleMultiplier(FlashLocation);
                            float ActualDuration = BlindDuration * ViewAngleMultiplier;
                            
                            // 최소 효과도 적용되는 경우에만 VFX 표시
                            if (ViewAngleMultiplier > 0.1f || ActualDuration > 0.0f)
                            {
                                float VFXDuration = FMath::Max(ActualDuration, MinimumFlashDuration);
                                Agent->Multicast_PlayNiagaraEffectAttached(Agent, FlashedTargetVFX, VFXDuration);
                            }
                        }
                    }
                }
            }
        }
    }
}