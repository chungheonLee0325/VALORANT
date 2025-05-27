#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "_DevTest/GA_LeftRight.h"
#include "Sage_Q_SlowOrb.generated.h"

class ABaseProjectile;

UCLASS()
class VALORANT_API USage_Q_SlowOrb : public UBaseGameplayAbility
{
    GENERATED_BODY()

    USage_Q_SlowOrb();

protected:
    virtual bool OnLeftClickInput() override;
}; 