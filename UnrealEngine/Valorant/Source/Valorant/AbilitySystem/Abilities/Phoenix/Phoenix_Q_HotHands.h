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
 
    virtual void HandleLeftClick(FGameplayEventData EventData) override;
    virtual void HandleRightClick(FGameplayEventData EventData) override;
    
    // === 내부 처리 함수들 ===
    // 던지기 타입별 처리
    void ExecuteStraightThrow();
    void ExecuteCurvedThrow();
    
    // 투사체 생성 (타입별)
    bool SpawnProjectileByType(EPhoenixQThrowType ThrowType);
    
    // === 정리 함수 ===
    virtual void CleanupAbility() override;
};