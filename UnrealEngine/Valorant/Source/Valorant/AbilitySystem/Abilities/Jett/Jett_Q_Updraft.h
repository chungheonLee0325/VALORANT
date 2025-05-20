#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Jett_Q_Updraft.generated.h"

UCLASS()
class VALORANT_API UJett_Q_Updraft : public UBaseGameplayAbility
{
    GENERATED_BODY()

    UJett_Q_Updraft();
    
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
}; 