#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "_DevTest/GA_Sample.h"
#include "Sage_Q_SlowOrb.generated.h"

class ABaseProjectile;

UCLASS()
//class VALORANT_API USage_Q_SlowOrb : public UBaseGameplayAbility
class VALORANT_API USage_Q_SlowOrb : public UGA_Sample
{
    GENERATED_BODY()

    USage_Q_SlowOrb();
public:
    UFUNCTION()
    bool SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo);

protected:
    virtual void Active_Left_Click(FGameplayEventData data) override;
    
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ABaseProjectile> ProjectileClass;
}; 