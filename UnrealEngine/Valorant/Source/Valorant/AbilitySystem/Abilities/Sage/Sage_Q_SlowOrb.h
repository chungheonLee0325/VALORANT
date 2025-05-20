#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "_DevTest/GA_LeftRight.h"
#include "Sage_Q_SlowOrb.generated.h"

class ABaseProjectile;

UCLASS()
//class VALORANT_API USage_Q_SlowOrb : public UBaseGameplayAbility
class VALORANT_API USage_Q_SlowOrb : public UGA_LeftRight
{
    GENERATED_BODY()

    USage_Q_SlowOrb();

protected:
    virtual void Active_Left_Click(FGameplayEventData data) override;
}; 