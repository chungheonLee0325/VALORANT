// Fill out your copyright notice in the Description page of Project Settings.


#include "Spike.h"

#include "Valorant.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameManager/MatchGameMode.h"
#include "Player/Agent/BaseAgent.h"
#include "UI/DetectWidget.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Weapon/ThirdPersonInteractor.h"


ASpike::ASpike()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinder<USkeletalMesh> SpikeMeshObj(
		TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Spike/Spike.Spike'"));
	if (SpikeMeshObj.Succeeded())
	{
		Mesh->SetSkeletalMesh(SpikeMeshObj.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.34f));

	// 인터랙터 타입 설정
	InteractorType = EInteractorType::Spike;
}

void ASpike::BeginPlay()
{
	Super::BeginPlay();

	if (const auto* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
	{
		DetectWidget->SetName(TEXT("획득 스파이크"));
	}

	// 게임 스테이트 캐싱
	CachedGameState = GetWorld()->GetGameState<AMatchGameState>();
}

void ASpike::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		// 설치 중인 경우 진행 상황 업데이트
		if (SpikeState == ESpikeState::Planting && InteractingAgent)
		{
			// 플레이어가 죽었는지 확인
			if (InteractingAgent->IsDead() || !InteractingAgent->IsSpikePlanting())
			{
				ServerRPC_CancelPlanting();
				return;
			}

			InteractProgress += DeltaTime;
			AddActorWorldOffset(FVector(0, 0, 12.66f) * DeltaTime);
			if (InteractProgress >= PlantTime)
			{
				ServerRPC_FinishPlanting();
			}
		}
		// 해제 중인 경우 진행 상황 업데이트
		else if (SpikeState == ESpikeState::Defusing && InteractingAgent)
		{
			// 플레이어가 죽었는지 확인
			if (InteractingAgent->IsDead())
			{
				ServerRPC_CancelDefusing();
				return;
			}

			InteractProgress += DeltaTime;
			AddActorWorldOffset(FVector(0, 0, -7.14f) * DeltaTime);

			// 반 해제 체크
			if (!bIsHalfDefused && InteractProgress >= HalfDefuseTime)
			{
				CheckHalfDefuse();
			}

			// 해제 완료 체크
			if (InteractProgress >= DefuseTime)
			{
				ServerRPC_FinishDefusing();
			}
		}
		// 설치 완료된 스파이크 폭발 타이머 업데이트
		else if (SpikeState == ESpikeState::Planted)
		{
			RemainingDetonationTime -= DeltaTime;
			if (RemainingDetonationTime <= 0)
			{
				ServerRPC_Detonate();
			}
		}
	}
}

void ASpike::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpike, SpikeState);
	DOREPLIFETIME(ASpike, InteractProgress);
	DOREPLIFETIME(ASpike, RemainingDetonationTime);
	DOREPLIFETIME(ASpike, InteractingAgent);
	DOREPLIFETIME(ASpike, bIsHalfDefused);
	DOREPLIFETIME(ASpike, LastDefusingAgent);
}

void ASpike::OnRep_SpikeState()
{
	// 스파이크 상태에 따른 시각적/청각적 효과 업데이트
	switch (SpikeState)
	{
	case ESpikeState::Dropped:
		// 떨어진 상태 - 시각적 표시 활성화
		// DetectWidgetComponent->SetVisibility(true);
		break;

	case ESpikeState::Carried:
		// 소지 상태 - 위젯 비활성화
		// DetectWidgetComponent->SetVisibility(false);
		break;

	case ESpikeState::Planting:
		// 설치 중 효과
		break;

	case ESpikeState::Planted:
		// 설치 완료 효과 (경고음, 불빛 등)
		break;

	case ESpikeState::Defusing:
		// 해제 중 효과
		break;

	case ESpikeState::Defused:
		// 해제 완료 효과
		break;
	}
}

void ASpike::ServerRPC_PickUp_Implementation(ABaseAgent* Agent)
{
	if (false == ServerOnly_CanAutoPickUp(Agent))
	{
		return;
	}
	Super::ServerRPC_PickUp_Implementation(Agent);
	
	Agent->AcquireInteractor(this);

	// 스파이크 Mesh 숨기기
	SetActive(false);

	// 스파이크 상태 업데이트
	SpikeState = ESpikeState::Carried;

	FAttachmentTransformRules AttachmentRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		true
		);
	Mesh->AttachToComponent(Agent->GetMesh1P(), AttachmentRules, FName(TEXT("R_WeaponSocket")));

	Mesh->AttachToComponent(Agent->GetMesh1P(), AttachmentRules, FName(TEXT("R_WeaponSocket")));
	if (nullptr != ThirdPersonInteractor)
	{
		ThirdPersonInteractor->Destroy();
		ThirdPersonInteractor = nullptr;
	}
	if ((ThirdPersonInteractor = GetWorld()->SpawnActor<AThirdPersonInteractor>()))
	{
		ThirdPersonInteractor->SetOwner(Agent);
		ThirdPersonInteractor->MulticastRPC_InitSpike(this);
		ThirdPersonInteractor->AttachToComponent(Agent->GetMesh(), AttachmentRules, FName(TEXT("R_WeaponSocket")));
	}
}

void ASpike::ServerRPC_Drop_Implementation()
{
	if (SpikeState != ESpikeState::Carried)
	{
		return;
	}
	OwnerAgent->ResetOwnSpike();
	Super::ServerRPC_Drop_Implementation();

	// 스파이크 Mesh 보이기
	SetActive(true);

	// 스파이크 상태 업데이트
	SpikeState = ESpikeState::Dropped;
}

void ASpike::ServerRPC_Interact_Implementation(ABaseAgent* InteractAgent)
{
	// NET_LOG(LogTemp, Warning, TEXT("%hs,스파이크 인터랙트 호출됨"), __FUNCTION__);
	if (!InteractAgent)
	{
		return;
	}

	auto* PS = InteractAgent->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}
	
	// 상태에 따른 상호작용 처리
	switch (SpikeState)
	{
	case ESpikeState::Dropped:
		// 떨어진 스파이크 - 공격팀만 주울 수 있음
		if (AMatchGameMode::IsAttacker(PS->bIsBlueTeam))
		{
			// 스파이크 줍기
			ServerRPC_PickUp_Implementation(InteractAgent);
		}
		break;

	case ESpikeState::Carried:
		// 이미 소지 중인 스파이크 - 공격팀은 설치 가능
		// 현재 라운드가 InRound 상태인지 확인
		if (OwnerAgent == InteractAgent && AMatchGameMode::IsAttacker(PS->bIsBlueTeam) && 
			IsInPlantZone() && IsGameStateInRound())
		{
			// 스파이크 설치 시작
			ServerRPC_StartPlanting(InteractAgent);
		}
		break;

	case ESpikeState::Planted:
		// 설치된 스파이크 - 수비팀만 해제 가능
		if (!AMatchGameMode::IsAttacker(PS->bIsBlueTeam))
		{
			// 스파이크 해제 시작
			ServerRPC_StartDefusing(InteractAgent);
		}
		break;

	default:
		break;
	}
}

void ASpike::ServerRPC_Cancel_Implementation(ABaseAgent* InteractAgent)
{
	if (!InteractAgent)
	{
		return;
	}

	auto* PS = InteractAgent->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}

	// 상태에 따른 상호작용 처리
	switch (SpikeState)
	{
	case ESpikeState::Planting:
		// 이미 설치 중인 스파이크
		if (OwnerAgent == InteractAgent && AMatchGameMode::IsAttacker(PS->bIsBlueTeam) && IsInPlantZone())
		{
			// 스파이크 설치 취소
			ServerRPC_CancelPlanting();
		}
		break;

	case ESpikeState::Defusing:
		// 해제 중인 스파이크
		if (!AMatchGameMode::IsAttacker(PS->bIsBlueTeam))
		{
			// 스파이크 해제 취소
			ServerRPC_CancelDefusing();
		}
		break;

	default:
		break;
	}
}

void ASpike::ServerRPC_StartPlanting_Implementation(ABaseAgent* Agent)
{
	if (!Agent || SpikeState != ESpikeState::Carried || OwnerAgent != Agent || !IsInPlantZone())
	{
		return;
	}
	
	// 현재 라운드가 InRound 상태인지 확인
	if (!IsGameStateInRound())
	{
		return;
	}
	
	// 스파이크 설치 시작
	SpikeState = ESpikeState::Planting;
	InteractingAgent = Agent;
	InteractProgress = 0.0f;

	// 스파이크 Mesh 보이기
	PlantingLocation = OwnerAgent->GetMovementComponent()->GetActorFeetLocation() + OwnerAgent->GetActorForwardVector()
		* 80 + FVector(0, 0, -50);
	SetActorLocation(PlantingLocation);
	SetActive(true);

	// 설치 시작 이벤트 발생
	MulticastRPC_OnPlantingStarted();
	MulticastRPC_AgentStartPlant(Agent);
}

void ASpike::ServerRPC_CancelPlanting_Implementation()
{
	if (SpikeState != ESpikeState::Planting)
	{
		return;
	}
	
	MulticastRPC_AgentCancelPlant(InteractingAgent);

	// 설치 취소
	SpikeState = ESpikeState::Carried;
	InteractingAgent = nullptr;
	InteractProgress = 0.0f;

	// 설치 취소 이벤트 발생
	MulticastRPC_OnPlantingCancelled();
}

void ASpike::ServerRPC_FinishPlanting_Implementation()
{
	if (SpikeState != ESpikeState::Planting || !InteractingAgent)
	{
		return;
	}

	// 설치 완료
	SpikeState = ESpikeState::Planted;
	InteractProgress = 0.0f;
	RemainingDetonationTime = 45.0f; // 폭발까지 45초
	
	// 게임 모드에 설치 완료 알림
	AMatchGameMode* GameMode = Cast<AMatchGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		AMatchPlayerController* PC = Cast<AMatchPlayerController>(InteractingAgent->GetController());
		if (PC)
		{
			GameMode->OnSpikePlanted(PC);
			GameMode->bSpikePlanted = true;
		}
	}

	// BaseInteractor::Drop에서 하던 일들을 직접 한다
	FDetachmentTransformRules DetachmentRule(
		EDetachmentRule::KeepWorld,
		EDetachmentRule::KeepWorld,
		EDetachmentRule::KeepRelative,
		true
	);
	Mesh->DetachFromComponent(DetachmentRule);
	
	// 스파이크 위치 고정
	const FVector& ForwardVector = OwnerAgent->GetActorForwardVector();
	const FVector& FeetLocation = OwnerAgent->GetMovementComponent()->GetActorFeetLocation();
	const FVector NewLocation = FeetLocation + ForwardVector * 50;
	SetActorLocation(NewLocation);
	SetActorRotation(FRotator(0, 0, 0));
	
	// 보상 지급
	InteractingAgent->RewardSpikeInstall();
	MulticastRPC_AgentFinishPlant(InteractingAgent);

	InteractingAgent = nullptr;
	SetOwnerAgent(nullptr);

	// 설치 완료 이벤트 발생
	MulticastRPC_OnPlantingFinished();
}

void ASpike::ServerRPC_StartDefusing_Implementation(ABaseAgent* Agent)
{
	if (!Agent || SpikeState != ESpikeState::Planted)
	{
		return;
	}

	// 수비팀만 해제 가능
	auto* PS = Agent->GetPlayerState<AAgentPlayerState>();
	if (!PS || AMatchGameMode::IsAttacker(PS->bIsBlueTeam))
	{
		return;
	}

	// 같은 에이전트가 다시 해제하는 경우 반 해제 적용
	// bool isHalfDefuse = bIsHalfDefused && LastDefusingAgent == Agent;

	// 스파이크 해제 시작
	SpikeState = ESpikeState::Defusing;
	InteractingAgent = Agent;

	// 반 해제 상태라면 진행도를 절반으로 설정
	InteractProgress = bIsHalfDefused ? HalfDefuseTime : 0.0f;

	// 해제 시작 이벤트 발생
	MulticastRPC_OnDefusingStarted(bIsHalfDefused);
}

void ASpike::ServerRPC_CancelDefusing_Implementation()
{
	if (SpikeState != ESpikeState::Defusing)
	{
		return;
	}

	// 해제 취소
	SpikeState = ESpikeState::Planted;
	InteractingAgent = nullptr;
	InteractProgress = 0.0f;
	
	FVector location = bIsHalfDefused ? PlantingLocation + FVector(0, 0, -25) : PlantingLocation;
	SetActorLocation(location);

	// 해제 취소 이벤트 발생
	MulticastRPC_OnDefusingCancelled();
}

void ASpike::ServerRPC_FinishDefusing_Implementation()
{
	if (SpikeState != ESpikeState::Defusing || !InteractingAgent)
	{
		return;
	}

	// 스파이크 Mesh 숨기기
	SetActive(false);

	// 해제 완료
	SpikeState = ESpikeState::Defused;

	// 게임 모드에 해제 완료 알림
	AMatchGameMode* GameMode = Cast<AMatchGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		AMatchPlayerController* PC = Cast<AMatchPlayerController>(InteractingAgent->GetController());
		if (PC)
		{
			GameMode->OnSpikeDefused(PC);
		}
	}

	// 반 해제 상태 리셋
	bIsHalfDefused = false;
	LastDefusingAgent = nullptr;

	InteractingAgent = nullptr;
	InteractProgress = 0.0f;

	// 해제 완료 이벤트 발생
	MulticastRPC_OnDefusingFinished();
}

void ASpike::ServerRPC_Detonate_Implementation()
{
	if (SpikeState != ESpikeState::Planted)
	{
		return;
	}

	// 폭발 처리
	// AMatchGameMode* GameMode = Cast<AMatchGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	// if (GameMode)
	// {
	// 	// 스파이크 폭발 시 공격팀 승리
	// 	GameMode->StartEndPhaseBySpikeActive();
	// }

	// 폭발 이벤트 발생
	MulticastRPC_OnDetonated();

	// 스파이크 제거 (폭발 이펙트 후 삭제)
	FTimerHandle DestroyTimerHandle;
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, [this]()
	{
		Destroy();
	}, 3.0f, false);
}

void ASpike::CheckHalfDefuse()
{
	if (SpikeState == ESpikeState::Defusing && InteractingAgent)
	{
		// 반 해제 상태 설정
		bIsHalfDefused = true;
		LastDefusingAgent = InteractingAgent;
	}
}

bool ASpike::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	if (!Agent || SpikeState != ESpikeState::Dropped)
	{
		return false;
	}

	const auto* PS = Agent->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return false;
	}

	// 공격팀만 스파이크를 주울 수 있음
	return AMatchGameMode::IsAttacker(PS->bIsBlueTeam);
}

bool ASpike::ServerOnly_CanDrop() const
{
	// 소지 중인 상태에서만 떨어뜨릴 수 있음
	return SpikeState == ESpikeState::Carried;
}

bool ASpike::ServerOnly_CanInteract() const
{
	if (!OwnerAgent)
	{
		return false;
	}
	
	const auto* PS = OwnerAgent->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return false;
	}
	
	// 공격팀이고 플랜트 영역에 있고 게임 상태가 InRound일 때만 설치 가능
	if (SpikeState == ESpikeState::Carried)
	{
		return AMatchGameMode::IsAttacker(PS->bIsBlueTeam) && IsInPlantZone() && IsGameStateInRound();
	}
	
	return false;
}

bool ASpike::IsInPlantZone() const
{
	// 임시 구현: 현재는 항상 설치 가능
	// 실제 구현에서는 플랜트 영역 콜리전 체크 필요
	return OwnerAgent->GetIsInPlantZone();
}

bool ASpike::IsGameStateInRound() const
{
	if (CachedGameState)
	{
		return CachedGameState->GetRoundSubState() == ERoundSubState::RSS_InRound;
	}
	return false;
}

void ASpike::MulticastRPC_OnPlantingStarted_Implementation()
{
	// 설치 시작 효과 (사운드, 애니메이션 등)
	HandlePlantingStarted();
}

void ASpike::MulticastRPC_OnPlantingCancelled_Implementation()
{
	// 설치 취소 효과
	HandlePlantingCancelled();
}

void ASpike::MulticastRPC_OnPlantingFinished_Implementation()
{
	// 설치 완료 효과
	HandlePlantingFinished();
}

void ASpike::MulticastRPC_OnDefusingStarted_Implementation(bool bHalfDefuse)
{
	// 해제 시작 효과
	HandleDefusingStarted();
	// bHalfDefuse가 true인 경우 반 해제 상태임을 표시
}

void ASpike::MulticastRPC_OnDefusingCancelled_Implementation()
{
	// 해제 취소 효과
	HandleDefusingCancelled();
}

void ASpike::MulticastRPC_OnDefusingFinished_Implementation()
{
	// 해제 완료 효과
	HandleDefusingFinished();
}

void ASpike::MulticastRPC_OnDetonated_Implementation()
{
	// 폭발 효과 (사운드, 파티클 등)
	HandleDetonated();
}

void ASpike::Destroyed()
{
	Super::Destroyed();

	if (OwnerAgent)
	{
		OwnerAgent->ResetOwnSpike();
	}
}

void ASpike::MulticastRPC_AgentStartPlant_Implementation(ABaseAgent* Agent)
{
	Agent->OnSpikeStartPlant();
}

void ASpike::MulticastRPC_AgentCancelPlant_Implementation(ABaseAgent* Agent)
{
	Agent->OnSpikeCancelPlant();	
}

void ASpike::MulticastRPC_AgentFinishPlant_Implementation(ABaseAgent* Agent)
{
	Agent->OnSpikeFinishPlant();
}
