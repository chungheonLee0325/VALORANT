#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "FlashComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFlashIntensityChanged, float, NewIntensity);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlashComponent();

	// 섬광 효과 시작
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void StartFlashEffect(float Intensity, float Duration);

	// 섬광 효과 중지
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void StopFlashEffect();

	// 현재 섬광 강도 가져오기
	UFUNCTION(BlueprintCallable, Category = "Flash")
	float GetCurrentFlashIntensity() const { return CurrentFlashIntensity; }

	// 섬광 상태인지 확인
	UFUNCTION(BlueprintCallable, Category = "Flash")
	bool IsFlashed() const { return bIsFlashed; }

	// 섬광 강도 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Flash")
	FOnFlashIntensityChanged OnFlashIntensityChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 섬광 업데이트
	void UpdateFlashEffect();

	// 현재 섬광 강도 (0.0 ~ 1.0)
	float CurrentFlashIntensity = 0.0f;

	// 목표 섬광 강도
	float TargetFlashIntensity = 0.0f;

	// 섬광 지속 시간
	float FlashDuration = 0.0f;

	// 경과 시간
	float ElapsedTime = 0.0f;

	// 섬광 상태
	bool bIsFlashed = false;

	// 섬광 회복 커브 (빠른 감소)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
	float FlashRecoveryExponent = 2.0f;
};
