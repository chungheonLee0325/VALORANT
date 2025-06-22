#pragma once

#include "CoreMinimal.h"
#include "ValorantObject/HandChargeActor/BaseOrbActor.h"
#include "HealingOrbActor.generated.h"

UCLASS()
class VALORANT_API AHealingOrbActor : public ABaseOrbActor
{
	GENERATED_BODY()

public:
	AHealingOrbActor();

	// 대상 하이라이트 설정
	UFUNCTION(BlueprintCallable, Category = "Healing Orb")
	void SetTargetHighlight(bool bHighlight);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 하이라이트 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* HighlightEffect;

	// 색상 설정
	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	FLinearColor NormalColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	FLinearColor HighlightColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

	// 하이라이트 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	class USoundBase* OrbHighlightSound;

private:
	// 하이라이트 상태 (복제됨)
	UPROPERTY(ReplicatedUsing = OnRep_IsHighlighted)
	bool bIsHighlighted = false;
    
	UFUNCTION()
	void OnRep_IsHighlighted();
    
	// 내부 하이라이트 업데이트 함수
	void UpdateHighlightVisuals();
};