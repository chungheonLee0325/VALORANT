// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniMapWidget.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/MapTestAgent.h"

// MinimapWidget 클래스 생성자 정의
UMiniMapWidget::UMiniMapWidget(const FObjectInitializer& ObjectInitializer)

// 부모 클래스 생성자 호출
: Super(ObjectInitializer)
{
	MapScale = 0.1f; // 기본 맵 스케일 설정 (1 월드 단위 = 0.1 미니맵 픽셀)
	MinimapSize = 256.0f; // 기본 미니맵 사이즈 설정 (256x256 픽셀)
	MinimapCenter = FVector2D(MinimapSize / 2, MinimapSize / 2); // 미니맵 중앙점 계산 (미니맵 크기의 절반)
}

// 위젯이 생성될 때 호출되는 함수
void UMiniMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 로컬 플레이어 컨트롤러 가져오기
	APlayerController* PC = GetOwningPlayer(); // 이 위젯을 소유하고 있는 플레이어 컨트롤러 가져오기
	if (PC) // 플레이어 컨트롤러가 유효한 경우
	{
		// 로컬 플레이어의 캐릭터 가져오기
		APawn* PlayerPawn = PC->GetPawn(); // 플레이어 컨트롤러가 제어하는 폰(캐릭터) 가져오기
		ObserverAgent = Cast<ABaseAgent>(PlayerPawn); // 폰을 BaseAgent로 형변환하여 저장
	}
}

void UMiniMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 매 프레임 아이콘 업데이트
	UpdateAgentIcons(); // 모든 에이전트 아이콘 업데이트 함수 호출
}


// 미니맵에 에이전트 추가 함수
void UMiniMapWidget::AddAgentToMinimap(ABaseAgent* Agent)
{
	if (IsValid(Agent) && !MappedAgents.Contains(Agent)) // 에이전트가 유효하고 아직 미니맵에 등록되지 않은 경우만
	{
		MappedAgents.Add(Agent); // 미니맵에 표시될 에이전트 목록에 추가
	}
}

// 미니맵에서 에이전트 제거 함수
void UMiniMapWidget::RemoveAgentFromMinimap(ABaseAgent* Agent)
{
	if (IsValid(Agent)) // 에이전트가 유효한 경우
	{
		MappedAgents.Remove(Agent); // 미니맵에 표시될 에이전트 목록에서 제거
	}
}


// 미니맵 스케일 설정 함수
void UMiniMapWidget::SetMinimapScale(float NewScale)
{
	if (NewScale > 0) // 새 스케일이 양수인 경우만 (0 이하는 유효하지 않음)
	{
		MapScale = NewScale; // 미니맵 스케일 업데이트
	}
}


// 월드 좌표를 미니맵 좌표로 변환하는 함수
FVector2D UMiniMapWidget::WorldToMinimapPosition(const FVector& WorldLocation)
{
	if (!IsValid(ObserverAgent)) // 관찰자 에이전트가 유효하지 않은 경우
		return MinimapCenter; // 기본값으로 미니맵 중앙 반환
    
	// 관찰자 위치와 회전
	FVector ObserverLocation = ObserverAgent->GetActorLocation(); // 관찰자의 월드 위치 가져오기
	float ObserverYaw = ObserverAgent->GetActorRotation().Yaw; // 관찰자의 Yaw 회전값 가져오기 (수평 회전)
    
	// 월드 좌표의 X, Y 차이 계산
	float DeltaX = WorldLocation.X - ObserverLocation.X; // X축 방향 거리 차이
	float DeltaY = WorldLocation.Y - ObserverLocation.Y; // Y축 방향 거리 차이
    
	// 관찰자 회전에 맞게 회전 변환 (관찰자 기준으로 미니맵 회전)
	float RotationRadians = FMath::DegreesToRadians(ObserverYaw); // 회전값을 라디안으로 변환
	float RotatedX = DeltaX * FMath::Cos(RotationRadians) + DeltaY * FMath::Sin(RotationRadians); // 회전 변환된 X 좌표
	float RotatedY = -DeltaX * FMath::Sin(RotationRadians) + DeltaY * FMath::Cos(RotationRadians); // 회전 변환된 Y 좌표
    
	// 미니맵 스케일 적용하여 미니맵 좌표 계산
	FVector2D MinimapPos; // 미니맵 좌표 변수
	MinimapPos.X = MinimapCenter.X + RotatedX * MapScale; // X 좌표 계산 (중앙에서의 오프셋)
	MinimapPos.Y = MinimapCenter.Y + RotatedY * MapScale; // Y 좌표 계산 (중앙에서의 오프셋)
    
	return MinimapPos; // 계산된 미니맵 좌표 반환
}

void UMiniMapWidget::UpdateAgentIcons()
{
	if (!IsValid(ObserverAgent)) // 관찰자 에이전트가 유효하지 않은 경우
		return; // 함수 종료
    
	// 모든 에이전트 위치 및 아이콘 업데이트
	for (ABaseAgent* Agent : MappedAgents) // 미니맵에 표시될 모든 에이전트에 대해 반복
	{
		if (!IsValid(Agent)) // 에이전트가 유효하지 않은 경우
			continue; // 다음 에이전트로 넘어감
        
		// 월드 위치를 미니맵 좌표로 변환
		FVector WorldLocation = Agent->GetActorLocation(); // 에이전트의 월드 위치 가져오기
		FVector2D MinimapPosition = WorldToMinimapPosition(WorldLocation); // 월드 위치를 미니맵 좌표로 변환
        
		// 에이전트 가시성 상태 확인
		EVisibilityState VisState = Agent->GetVisibilityStateForAgent(ObserverAgent); // 현재 관찰자에게 보이는 상태 확인
        
		// 적절한 아이콘 선택
		UTexture2D* IconToUse = nullptr; // 사용할 아이콘 텍스처
        
		switch (VisState) // 가시성 상태에 따라 다른 아이콘 선택
		{
		case EVisibilityState::Visible: // 보이는 상태인 경우
			IconToUse = Agent->GetMinimapIcon(); // 에이전트의 기본 아이콘 사용
			break;
		case EVisibilityState::QuestionMark: // 물음표 상태인 경우
			IconToUse = Agent->GetQuestionMarkIcon(); // 물음표 아이콘 사용
			break;
		case EVisibilityState::Hidden: // 숨김 상태인 경우
			IconToUse = nullptr; // 아이콘 없음 (null)
			break;
		}
        
		// 아이콘 업데이트 (블루프린트에서 구현)
		UpdateAgentIcon(Agent, MinimapPosition, IconToUse, VisState); // 블루프린트에서 구현된 함수 호출하여 UI 업데이트
	}
}

