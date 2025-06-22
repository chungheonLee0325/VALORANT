#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarrierWallActor.generated.h"

UCLASS()
class VALORANT_API ABarrierWallActor : public AActor
{
    GENERATED_BODY()

public:
    ABarrierWallActor();

    // 장벽 초기화
    UFUNCTION(BlueprintCallable, Category = "Barrier")
    void InitializeBarrier(float SegmentHealth, float Lifespan, float BuildTime);

    // 미리보기 모드 설정
    UFUNCTION(BlueprintCallable, Category = "Barrier")
    void SetPreviewMode(bool bPreview);
    
    // 미리보기 모드 확인
    UFUNCTION(BlueprintPure, Category = "Barrier")
    bool IsPreviewMode() const { return bIsPreviewMode; }
    
    // 설치 가능 여부 표시 (인터페이스 호환성 유지)
    UFUNCTION(BlueprintCallable, Category = "Barrier")
    void SetPlacementValid(bool bValid);

    // 데미지 처리
    UFUNCTION()
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
                            class AController* EventInstigator, AActor* DamageCauser) override;

    void StartBuild(){StartBuildAnimation();}
    void SetOnlyOwnerSee(bool bCond);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 건설 애니메이션
    UFUNCTION()
    void StartBuildAnimation();
    
    UFUNCTION()
    void CompleteBuild();
    
    // 세그먼트 파괴
    UFUNCTION(NetMulticast,Reliable)
    void NetMulti_DestroySegment(int32 SegmentIndex);
    
    UFUNCTION()
    void DestroyBarrier();

    // 컴포넌트들
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* RootSceneComponent;
    
    // 4개의 세그먼트 (왼쪽부터 오른쪽으로)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* SegmentMesh1;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* SegmentMesh2;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* SegmentMesh3;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* SegmentMesh4;
    
    // 충돌 박스들
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* SegmentCollision1;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* SegmentCollision2;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* SegmentCollision3;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* SegmentCollision4;

    // 세그먼트 체력
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier State")
    TArray<float> SegmentHealthArray;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier State")
    TArray<bool> SegmentDestroyedArray;

    // 설정값들
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float DefaultSegmentHealth = 800.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float DefaultLifespan = 40.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float DefaultBuildTime = 2.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float SegmentWidth = 400.f;  // 각 세그먼트 너비
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float SegmentHeight = 400.f;  // 세그먼트 높이
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float SegmentThickness = 100.f;  // 세그먼트 두께

    // 머티리얼들
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* BarrierMaterial;
    
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* BarrierDamagedMaterial;
    
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* PreviewMaterial;  // 미리보기용 반투명 머티리얼

    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* BuildEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* DestroyEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* DamageEffect;

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* BuildSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* DamageSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* DestroySound;

private:
    bool bIsBuilding = true;
    bool bIsPreviewMode = false;
    float BuildProgress = 0.f;
    float BuildDuration = 2.0f;
    
    FTimerHandle BuildTimerHandle;
    FTimerHandle LifespanTimerHandle;
    
    // 각 세그먼트 컴포넌트에 쉽게 접근하기 위한 배열
    UPROPERTY()
    TArray<UStaticMeshComponent*> SegmentMeshes;
    
    UPROPERTY()
    TArray<UBoxComponent*> SegmentCollisions;

    void SetSegmentsFinalPosition();
    float GetSegmentXPosition(int32 SegmentIndex) const;
};