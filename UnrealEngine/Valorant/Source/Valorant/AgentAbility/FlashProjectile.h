#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "FlashProjectile.generated.h"

class UGameplayEffect;
class ABaseAgent;
class UParticleSystem;
class USoundBase;

UCLASS()
class VALORANT_API AFlashProjectile : public ABaseProjectile
{
    GENERATED_BODY()

public:
    AFlashProjectile();

protected:
    virtual void BeginPlay() override;
    
    // BaseProjectile의 OnProjectileBounced 오버라이드
    virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;

    // 섬광 폭발 처리
    UFUNCTION(BlueprintCallable, Category = "Flash")
    void ExplodeFlash();

    // 플레이어가 섬광 범위에 있는지 체크
    bool IsPlayerInFlashRange(ABaseAgent* Player, float& OutIntensity);

    // 시야 차단 체크 (벽 뒤에 있으면 섬광 안됨)
    bool HasLineOfSight(ABaseAgent* Player);

    // 섬광 효과 적용
    void ApplyFlashEffectToPlayer(ABaseAgent* Player, float Intensity);

private:
    // 섬광 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float FlashRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float FlashDuration = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MaxFlashIntensity = 1.0f;

    // 폭발 대기 시간 (발로란트 스타일)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float DetonationDelay = 0.5f;

    // 이펙트 및 사운드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* ExplosionSound;

    // 섬광 효과용 GameplayEffect 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UGameplayEffect> FlashGameplayEffect;

    // 폭발 예약된 상태인지
    bool bIsScheduledToExplode = false;
};