#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Jett_C_Cloudburst.generated.h"

UCLASS()
class VALORANT_API UJett_C_Cloudburst : public UBaseGameplayAbility
{
    GENERATED_BODY()

    UJett_C_Cloudburst();

    void HandleInstantAbility() override;
    virtual void HandleExecutingState() override;
    
    UPROPERTY(EditDefaultsOnly)
    UAnimMontage* AbilityMontage = nullptr;
}; 