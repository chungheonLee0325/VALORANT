// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniMapWidget.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API UMiniMapWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 미니맵 배경 이미지  (BP 바인딩)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UImage* MinimapBackground;

	// 미니맵 원점 좌표 (월드 좌표계의 어느 위치가 미니맵의 중심인지)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="MiniMap")
	FVector2D MapOrigin;

	// 미니맵 스케일 (월드 좌표를 미니맵 좌표로 변환 할때 사용)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="MiniMap")
	float MapScale = 1.0f;

	// 월드 좌표를 미니맵 좌표로 변환하는 함수
	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	FVector2D WorldMinimapPosition(const FVector& WorldPosition) const;

	// 미니맵 아이콘 업데이트
	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	void UpdateMinimapIcons();
	
};
