#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Phoenix_Q_HotHands.generated.h"

UCLASS()
class VALORANT_API UPhoenix_Q_HotHands : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UPhoenix_Q_HotHands();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ThrowMontage;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* CancelMontage;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* HandFireEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* ReadySound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* ThrowSound;
    // 핸드 이펙트 컴포넌트 참조
    UPROPERTY()
    UParticleSystemComponent* HandFireComponent;
    
    // 오버라이드 메서드
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                              const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo,
                              const FGameplayEventData* TriggerEventData) override;

    virtual void Active_Left_Click(FGameplayEventData data) override;

    
}; 