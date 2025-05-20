#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Jett_C_Cloudburst.generated.h"

UCLASS()
class VALORANT_API UJett_C_Cloudburst : public UBaseGameplayAbility
{
    GENERATED_BODY()

    UJett_C_Cloudburst();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    UPROPERTY(EditDefaultsOnly)
    UAnimMontage* AbilityMontage = nullptr;
}; 