// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniMapWidget.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/MapTestAgent.h"

void UMiniMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 매 틱마다 미니맵 아이콘 업데이트
	UpdateMinimapIcons();
}

// 월드 좌표를 미니맵 UI로 변환
FVector2D UMiniMapWidget::WorldMinimapPosition(const FVector& WorldPosition) const
{
	// 월드 좌표를 2D 미니맵 좌표로 변환
	// X,Y 좌표만 사용 (z축)은 무시
	FVector2D MapPosition;

	// X 좌표 변환 - 월드 x좌표에서 미니맵 원점 x를 빼고 스케일로 나눔
	MapPosition.X = (WorldPosition.X - MapOrigin.X ) / MapScale;
	// Y 좌표 변환 - 월드 Y좌표에서 미니맵 원점 y를 빼고 스케일로 나눔
	MapPosition.Y = (WorldPosition.Y - MapOrigin.Y ) / MapScale;

	// 변환된 미니맵 위치 반환
	return MapPosition;
}

// 미니맵 아이콘 업데이트 
void UMiniMapWidget::UpdateMinimapIcons()
{
	// 로컬 플레이어 가져오기 
	APlayerController* PlayerController = GetOwningPlayer();
	// 플레이어 컨트롤러가 없으면 중단 
	if (!PlayerController)
	{
		return;
	}

	// 로컬 플레이어 캐릭터 가져오기
	AMapTestAgent* LocalPlayer = Cast<AMapTestAgent>(PlayerController->GetPawn());
	if (!LocalPlayer)
	{
		return;
	}
	
	// 로컬 플레이어의 팀 ID 가져오기
	int32 LocalTeamID = LocalPlayer->TeamID;

	// 월드의 모든 에이전트 찾기
	TArray<AActor*> AllAgents;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),ABaseAgent::StaticClass(),AllAgents);
	
}




