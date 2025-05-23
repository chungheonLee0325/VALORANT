#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "FlashPostProcessComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UFlashPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlashPostProcessComponent();

	UFUNCTION(BlueprintCallable, Category = "Flash")
	void UpdateFlashPostProcess(float FlashIntensity);

protected:
	virtual void BeginPlay() override;

private:
	// 카메라 컴포넌트 참조
	UPROPERTY()
	UCameraComponent* OwnerCamera;

	// 원본 카메라 설정 저장
	FPostProcessSettings OriginalSettings;
	bool bOriginalSettingsSaved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
	float MaxBrightness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
	float MaxSaturation = 0.1f;
};