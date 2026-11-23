// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniMapWidget.h"

#include "Valorant.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Valorant/Player/Agent/BaseAgent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerState.h"

// 위젯이 생성될 때 호출되는 함수
void UMiniMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const auto* MyPC = GetWorld()->GetFirstPlayerController();
	if (nullptr == MyPC)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MyPC is NULL"), __FUNCTION__);
		return;
	}
	
	auto* MyPS = MyPC->GetPlayerState<AAgentPlayerState>();
	MyPlayerState = MyPS;

	if (MinimapBackground)
	{
		// 가시성 설정 - 디버깅을 위해 완전 가시로 변경
		MinimapBackground->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("미니맵 배경 가시성 설정: Visible"));
	}

	if (IconContainer)
	{
		IconContainer->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("아이콘 컨테이너 가시성 설정: Visible"));
	}

	// 초기 에이전트 스캔 수행
	ScanPlayer();
	GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UMiniMapWidget::Scan, ScanInterval, true);
}

void UMiniMapWidget::Scan()
{
	ScanPlayer();
	UpdateAgentIcons(); // 모든 에이전트 아이콘 업데이트 함수 호출
}

// 에이전트 자동 스캔 함수 구현
void UMiniMapWidget::ScanPlayer()
{
	// 월드의 모든 BaseAgent 검색
	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAgentPlayerState::StaticClass(), FoundPlayers);
    
	// 아직 등록되지 않은 플레이어만 추가
	for (AActor* Actor : FoundPlayers)
	{
		const AAgentPlayerState* Player = Cast<AAgentPlayerState>(Actor);
		AddPlayerToMinimap(Player); // 유효성 검사는 안에서 한다
	}
}

// 미니맵에 에이전트 추가 함수
void UMiniMapWidget::AddPlayerToMinimap(const AAgentPlayerState* Player)
{
	// 에이전트가 유효하고 아직 미니맵에 등록되지 않은 경우만
	if (Player && !PlayerArray.Contains(Player))
	{
		const bool bIsMe = MyPlayerState == Player;
		const bool bSameTeam = MyPlayerState->bIsBlueTeam == Player->bIsBlueTeam;
		
		const auto* MyAgent = MyPlayerState->GetPawn<ABaseAgent>();
		const auto* OtherAgent = Player->GetPawn<ABaseAgent>();
		if (MyAgent && OtherAgent)
		{
			const bool bIsDead = OtherAgent->IsDead();
			EVisibilityState VisState;
			const UTexture2D* IconToUse = nullptr;
			
			if ((bIsMe || bSameTeam || MyAgent->IsInFrustum(OtherAgent)) && false == bIsDead)
			{
				VisState = EVisibilityState::Visible;
				IconToUse = OtherAgent->GetMinimapIcon();
			}
			else
			{
				VisState = EVisibilityState::Hidden;
				IconToUse = nullptr;
			}
			PlayerArray.Add(Player);
			CreateAgentIcon(Player, IconToUse, VisState, bIsMe ? 0 : bSameTeam ? 1 : 2);
		}
		else
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, MyAgent or OtherAgent is nullptr"), __FUNCTION__);
		}
	}
}

// 미니맵에서 에이전트 제거 함수
void UMiniMapWidget::RemovePlayerFromMinimap(AAgentPlayerState* Player)
{
	if (Player)
	{
		PlayerArray.Remove(Player); // 미니맵에 표시될 에이전트 목록에서 제거

		// 해당 에이전트의 아이콘도 AgentIconMap에서 제거
		if (UUserWidget** FoundIcon = AgentIconMap.Find(Player))
		{
			if (IsValid(*FoundIcon))
			{
				(*FoundIcon)->RemoveFromParent();
			}
			AgentIconMap.Remove(Player);
		}
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
FVector2D UMiniMapWidget::WorldToMinimapPosition(const FVector& TargetActorLocation)
{
	const auto* MyAgent = MyPlayerState->GetPawn<ABaseAgent>();
	if (!IsValid(MyAgent)) // 관찰자 에이전트가 유효하지 않은 경우
		return MinimapCenter; // 기본값으로 미니맵 중앙 반환
    
	// 미니맵 스케일 적용하여 미니맵 좌표 계산
	FVector2D MinimapPos; // 미니맵 좌표 변수
	MinimapPos.X = (TargetActorLocation.Y + 8400.f) / 16800.f * 450.0f;
	MinimapPos.Y = (1.f - (TargetActorLocation.X / 16800.f)) * 450.f;
    MinimapPos += ConvertOffset;
	
	return MinimapPos; // 계산된 미니맵 좌표 반환
}


void UMiniMapWidget::UpdateAgentIcons()
{
	// A player who leaves keeps a destroyed PlayerState here until garbage collection nulls the entry.
	PlayerArray.RemoveAll([](const AAgentPlayerState* Player) { return !IsValid(Player); });
	for (auto Icon = AgentIconMap.CreateIterator(); Icon; ++Icon)
	{
		if (!IsValid(Icon->Key))
		{
			if (IsValid(Icon->Value))
			{
				Icon->Value->RemoveFromParent();
			}
			Icon.RemoveCurrent();
		}
	}

	const auto* MyAgent = MyPlayerState->GetPawn<ABaseAgent>();
	if (nullptr == MyAgent) // 관찰자 에이전트가 유효하지 않은 경우
		return; // 함수 종료

	// 1. 내 팀원 목록 캐싱
	TArray<const ABaseAgent*> TeamAgents;
	for (const auto* PS : PlayerArray)
	{
		const auto* OtherAgent = PS->GetPawn<ABaseAgent>();
		if (!IsValid(OtherAgent)) continue;
		if (OtherAgent == MyAgent) continue;
		if (OtherAgent->IsDead()) continue;
		if (OtherAgent->IsBlueTeam() == MyAgent->IsBlueTeam()) // 같은 팀
			TeamAgents.Add(OtherAgent);
	}

	// 모든 에이전트 위치 및 아이콘 업데이트
	for (const auto* PS : PlayerArray) // 미니맵에 표시될 모든 에이전트에 대해 반복
	{
		const auto* OtherAgent = PS->GetPawn<ABaseAgent>();
		if (!IsValid(OtherAgent)) continue;

		FVector TargetActorLocation = OtherAgent->GetActorLocation(); // 에이전트의 월드 위치 가져오기
		FVector2D ConvertedMinimapPosition = WorldToMinimapPosition(TargetActorLocation); // 월드 위치를 미니맵 좌표로 변환

		EVisibilityState VisState = EVisibilityState::Hidden;
		UTexture2D* IconToUse = nullptr;
		const bool bIsMe = MyAgent == OtherAgent;
		const bool bSameTeam = MyAgent->IsBlueTeam() == OtherAgent->IsBlueTeam();
		const bool bIsDead = OtherAgent->IsDead();

		// 1. 내가 직접 본 경우
		bool bVisible = (OtherAgent->GetVisibilityStateForAgent(MyAgent) == EVisibilityState::Visible);

		// 2. 내 팀원이 본 경우(아군 시야 공유)
		if (!bVisible && !bIsMe && !bSameTeam && !bIsDead) // 적만 체크
		{
			for (const auto* Teammate : TeamAgents)
			{
				if (OtherAgent->GetVisibilityStateForAgent(Teammate) == EVisibilityState::Visible)
				{
					bVisible = true;
					break;
				}
			}
		}

		if ((bIsMe || bSameTeam) && !bIsDead)
		{
			// 내 자신, 아군은 항상 보임
			IconToUse = OtherAgent->GetMinimapIcon();
			VisState = EVisibilityState::Visible;
		}
		else if (bVisible && !bIsDead)
		{
			// 적이지만 아군이 봤으므로 보임
			IconToUse = OtherAgent->GetMinimapIcon();
			VisState = EVisibilityState::Visible;
		}
		else
		{
			IconToUse = nullptr;
			VisState = EVisibilityState::Hidden;
		}

		// 아이콘 업데이트 (블루프린트에서 구현)
		UpdateAgentIcon(PS, ConvertedMinimapPosition, IconToUse, VisState, bIsMe ? 0 : bSameTeam ? 1 : 2); // 블루프린트에서 구현된 함수 호출하여 UI 업데이트
	}
}