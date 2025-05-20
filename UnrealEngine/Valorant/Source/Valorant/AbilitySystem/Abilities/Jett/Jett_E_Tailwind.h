#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Jett_E_Tailwind.generated.h"

UCLASS()
class VALORANT_API UJett_E_Tailwind : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UJett_E_Tailwind();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
}; 