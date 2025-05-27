#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "_DevTest/GA_LeftRight.h"
#include "Sage_E_HealingOrb.generated.h"

UCLASS()
class VALORANT_API USage_E_HealingOrb : public UBaseGameplayAbility
{
    GENERATED_BODY()

    USage_E_HealingOrb();

protected:
    // 힐링에 사용할 변수들
    UPROPERTY(EditDefaultsOnly, Category="Heal")
    float HealAmount = 100.f;
    UPROPERTY(EditDefaultsOnly, Category="Heal")
    float HealDuration = 2.f;
    UPROPERTY(EditDefaultsOnly, Category="Heal")
    float HealTickInterval = 0.2f;
    UPROPERTY(EditDefaultsOnly, Category="Heal")
    float HealTraceDistance = 1000.f;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
}; 