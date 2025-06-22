#pragma once

#include "CoreMinimal.h"
#include "ValorantObject/HandChargeActor/BaseOrbActor.h"
#include "BarrierOrbActor.generated.h"

UCLASS()
class VALORANT_API ABarrierOrbActor : public ABaseOrbActor
{
    GENERATED_BODY()

public:
    ABarrierOrbActor();

    // 설치 가능 여부 표시
    UFUNCTION(BlueprintCallable, Category = "Barrier Orb")
    void SetPlacementValid(bool bValid);

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    // 설치 가능 상태 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsPlacementValid)
    bool bIsPlacementValid = true;
    
    UFUNCTION()
    void OnRep_IsPlacementValid();
    
    // 설치 가능 여부 업데이트
    void UpdatePlacementVisuals();
};