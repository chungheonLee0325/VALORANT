// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniMapWidget.generated.h"


class ABaseAgent;
/**
 * 
 */
UCLASS()
class VALORANT_API UMiniMapWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	UMiniMapWidget(const FObjectInitializer& ObjectInitializer); // 생성자 선언
    
	virtual void NativeConstruct() override; // 위젯이 생성될 때 호출되는 함수, 부모 클래스 함수 오버라이드
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override; // 매 프레임 호출되는 함수, 부모 클래스 함수 오버라이드

	// 미니맵에 에이전트 추가 함수
    UFUNCTION(BlueprintCallable, Category = "Minimap") 
    void AddAgentToMinimap(ABaseAgent* Agent);
    
    // 미니맵에서 에이전트 제거 함수
    UFUNCTION(BlueprintCallable, Category = "Minimap")
    void RemoveAgentFromMinimap(ABaseAgent* Agent); 
    
	// 미니맵의 스케일을 설정하는 함수 (월드 단위 -> 미니맵 픽셀 변환 비율)
    UFUNCTION(BlueprintCallable, Category = "Minimap") 
    void SetMinimapScale(float NewScale); 
    
    // 미니맵 업데이트 - 월드 좌표를 미니맵 좌표로 변환하는 함수 (월드 좌표 -> 미니맵 좌표 변환)
    UFUNCTION(BlueprintCallable, Category = "Minimap") 
    FVector2D WorldToMinimapPosition(const FVector& WorldLocation); 
    
protected:
    // 미니맵에 표시될 모든 에이전트
    UPROPERTY() 
    TArray<ABaseAgent*> MappedAgents; 
    
    // 현재 미니맵을 보고 있는 플레이어의 에이전트
    UPROPERTY() 
    ABaseAgent* ObserverAgent; 
    
    // 미니맵 배경 이미지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), Category = "Minimap") 
    class UImage* MinimapBackground; // 미니맵 배경 이미지 위젯
    
    // 미니맵 스케일 - 월드 좌표를 미니맵 좌표로 변환하는 비율 (월드 단위 -> 미니맵 픽셀)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float MapScale; 
    
    // 미니맵 사이즈 (픽셀)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float MinimapSize; // 미니맵의 픽셀 크기
    
    // 미니맵 중앙점 -미니맵의 중앙 좌표 (픽셀)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    FVector2D MinimapCenter; 
    
    // 모든 에이전트의 미니맵 아이콘 위치와 상태를 업데이트하는 함수
    UFUNCTION() 
    void UpdateAgentIcons(); 
    
    // 미니맵에 아이콘 생성 함수 (블루프린트에서 구현)
    UFUNCTION(BlueprintImplementableEvent, Category = "Minimap") 
    void CreateAgentIcon(ABaseAgent* Agent, FVector2D Position, UTexture2D* IconTexture, EVisibilityState VisState); 
    
    // 미니맵 아이콘 업데이트 함수 (블루프린트에서 구현)
    UFUNCTION(BlueprintImplementableEvent, Category = "Minimap") 
    void UpdateAgentIcon(ABaseAgent* Agent, FVector2D Position, UTexture2D* IconTexture, EVisibilityState VisState);
	
};
