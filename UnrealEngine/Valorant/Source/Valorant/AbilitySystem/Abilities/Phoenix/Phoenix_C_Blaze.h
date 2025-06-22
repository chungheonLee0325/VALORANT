#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "AgentAbility/Phoenix/Phoenix_C_BlazeProjectile.h"
#include "Phoenix_C_Blaze.generated.h"

UCLASS()
class VALORANT_API UPhoenix_C_Blaze : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UPhoenix_C_Blaze();

protected:
    // 벽 투사체 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
    TSubclassOf<class APhoenix_C_BlazeProjectile> BlazeProjectileClass;
    
    // 오버라이드 함수들
    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
    virtual void ExecuteAbility() override;
    
private:
    // 벽 투사체 생성
    bool SpawnBlazeProjectile(EBlazeMovementType MovementType);
};