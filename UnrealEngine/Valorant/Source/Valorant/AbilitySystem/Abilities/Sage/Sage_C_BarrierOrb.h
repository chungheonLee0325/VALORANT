#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "_DevTest/GA_LeftRight.h"
#include "Sage_C_BarrierOrb.generated.h"

UCLASS()
class VALORANT_API USage_C_BarrierOrb : public UGA_LeftRight
{
    GENERATED_BODY()

    USage_C_BarrierOrb();

protected:
    // 장벽 생성에 사용할 변수들
    UPROPERTY(EditDefaultsOnly, Category="Barrier")
    TSubclassOf<AActor> BarrierClass;
    UPROPERTY(EditDefaultsOnly, Category="Barrier")
    float BarrierSpawnDistance = 300.f;
    UPROPERTY(EditDefaultsOnly, Category="Barrier")
    float BarrierSpawnHeight = 0.f;
    UPROPERTY(EditDefaultsOnly, Category="Barrier")
    float BarrierYawStep = 90.f;
    FRotator BarrierRotation;

    virtual void Active_Left_Click(FGameplayEventData data) override;
    virtual void Active_Right_Click(FGameplayEventData data) override;
}; 