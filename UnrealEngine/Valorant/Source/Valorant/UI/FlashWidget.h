#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "FlashWidget.generated.h"

UCLASS()
class VALORANT_API UFlashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 섬광 강도 업데이트
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void UpdateFlashIntensity(float Intensity);

	// 섬광 효과 시작
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void StartFlashEffect(float Duration);

	// 섬광 효과 중지
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void StopFlashEffect();

protected:
	virtual void NativeConstruct() override;

	// 화면을 덮는 하얀 이미지
	UPROPERTY(meta = (BindWidget))
	UImage* FlashOverlay;

	// 현재 섬광 강도
	UPROPERTY(BlueprintReadOnly, Category = "Flash")
	float CurrentFlashIntensity = 0.0f;
};