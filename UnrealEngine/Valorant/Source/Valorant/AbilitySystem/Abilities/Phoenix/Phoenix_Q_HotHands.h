#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Phoenix_Q_HotHands.generated.h"

UENUM(BlueprintType)
enum class EPhoenixQThrowType : uint8
{
    None,
    Straight,        // 좌클릭 - 직선 던지기
    Curved          // 우클릭 - 포물선 던지기
};

UCLASS()
class VALORANT_API UPhoenix_Q_HotHands : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UPhoenix_Q_HotHands();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EPhoenixQThrowType CurrentThrowType = EPhoenixQThrowType::None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ExecutingUnderThrow3pMontage = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ExecutingUnderThrow1pMontage = nullptr;

    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
    
    // 투사체 생성 (타입별)
    bool SpawnProjectileByType(EPhoenixQThrowType ThrowType);
};