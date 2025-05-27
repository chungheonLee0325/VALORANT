#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Jett_Q_Updraft.generated.h"

UCLASS()
class VALORANT_API UJett_Q_Updraft : public UBaseGameplayAbility
{
    GENERATED_BODY()

    UJett_Q_Updraft();
    
    virtual void ExecuteAbility();
}; 