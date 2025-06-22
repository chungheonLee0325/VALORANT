#pragma once

#include "AgentAbility/BaseProjectile.h"
#include "Phoenix_C_BlazeProjectile.generated.h"

UENUM(BlueprintType)
enum class EBlazeMovementType : uint8
{
    Straight,
    Curved
};

UCLASS()
class VALORANT_API APhoenix_C_BlazeProjectile : public ABaseProjectile
{
    GENERATED_BODY()

public:
    APhoenix_C_BlazeProjectile();

    // 스플라인 벽 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    TSubclassOf<class APhoenix_C_BlazeSplineWall> SplineWallClass;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    float SegmentSpawnInterval = 0.05f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    float MaxWallLength = 2000.0f;
    
    // 이동 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float StraightSpeed = 1500.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float CurvedSpeed = 1200.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float CurveRate = 45.0f;

    void SetMovementType(EBlazeMovementType Type);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SetMovementType(EBlazeMovementType Type);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(Replicated)
    EBlazeMovementType MovementType = EBlazeMovementType::Straight;
    
    UPROPERTY(Replicated)
    bool bIsActive = true;

private:
    UPROPERTY()
    class APhoenix_C_BlazeSplineWall* SplineWall;
    
    FTimerHandle SplineUpdateTimer;
    float TotalDistanceTraveled = 0.0f;
    float CurrentCurveAngle = 0.0f;
    
    void UpdateSplinePoint();
    void ApplyCurveMovement(float DeltaTime);
    void StopProjectile();
};