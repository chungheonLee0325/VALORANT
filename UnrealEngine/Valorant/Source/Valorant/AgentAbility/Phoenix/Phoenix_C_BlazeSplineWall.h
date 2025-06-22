#pragma once

#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Phoenix_C_BlazeSplineWall.generated.h"

class USplineMeshComponent;
class UBoxComponent;
class ABaseAgent;

UCLASS()
class VALORANT_API APhoenix_C_BlazeSplineWall : public AActor
{
    GENERATED_BODY()

public:
    APhoenix_C_BlazeSplineWall();

    // 스플라인 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
    USplineComponent* WallSpline;

    // 벽 설정값
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallHeight = 340.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallThickness = 50.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallDuration = 8.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float DamagePerSecond = 30.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float HealPerSecond = 6.25f;
    
    // 충돌 범위 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float FlashRangeMultiplier = 1.0f;  // 섬광 효과 범위 (기본 벽 두께)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float DamageRangeMultiplier = 2.0f;  // 데미지/힐 효과 범위 (벽 두께의 2배)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    UStaticMesh* WallMesh;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    UMaterialInterface* WallMaterial;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    TSubclassOf<class UGameplayEffect> GameplayEffect;

    // 스플라인 포인트 추가
    UFUNCTION(BlueprintCallable, Category = "Wall")
    void AddSplinePoint(const FVector& Location);
    
    // 벽 생성 완료
    UFUNCTION(BlueprintCallable, Category = "Wall")
    void FinalizeWall();
    
    // 멀티캐스트 함수들
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_AddSplinePoint(FVector Location);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_FinalizeWall();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY()
    TArray<USplineMeshComponent*> SplineMeshComponents;
    
    // 두 가지 충돌 범위를 위한 컴포넌트들
    UPROPERTY()
    TArray<UBoxComponent*> FlashCollisionComponents;  // 섬광 효과용 (가까운 범위)
    
    UPROPERTY()
    TArray<UBoxComponent*> DamageCollisionComponents;  // 데미지/힐용 (넓은 범위)
    
    // 각 범위에 있는 에이전트들 추적
    UPROPERTY()
    TSet<ABaseAgent*> FlashOverlappedAgents;
    
    UPROPERTY()
    TSet<ABaseAgent*> DamageOverlappedAgents;
    
    FTimerHandle DamageTimerHandle;
    FTimerHandle FlashTimerHandle;
    FTimerHandle DurationTimerHandle;
    
    float EffectApplicationInterval = 0.25f;
    float DamagePerTick = -7.5f;
    float HealPerTick = 1.5625f;
    
    // 스플라인 메시 업데이트
    void UpdateSplineMesh();
    
    // 섬광 충돌 처리
    UFUNCTION()
    void OnFlashBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnFlashEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    
    // 데미지 충돌 처리
    UFUNCTION()
    void OnDamageBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnDamageEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    
    void ApplyDamageEffect();
    void ApplyFlashEffect();
    void OnElapsedDuration();
    bool IsPhoenixSelf(AActor* Actor) const;
    void CalculateTickValues();
};