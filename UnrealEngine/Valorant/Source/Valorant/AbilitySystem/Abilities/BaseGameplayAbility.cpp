// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameplayAbility.h"

#include <GameManager/SubsystemSteamManager.h>

#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AgentAbility/BaseProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UBaseGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
	// 기본 활성화 조건 확인
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	//어빌리티 ID가 유효하고 스택 소비가 필요한 경우
	if (m_AbilityID > 0)
	{
		// 플레이어 스테이트 가져오기
		AAgentPlayerState* PS = Cast<AAgentPlayerState>(ActorInfo->PlayerController->PlayerState);
		if (PS == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("어빌리티 활성화 확인 실패: PlayerState가 NULL입니다."));
			return false;
		}

		// 현재 스택 확인
		int32 CurrentStack = PS->GetAbilityStack(m_AbilityID);
		CurrentStack = GetAbilityStack(ActorInfo->PlayerController.Get());

		// 스택이 없으면 활성화 불가
		if (CurrentStack <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 없어 활성화할 수 없습니다."), m_AbilityID);
			return false;
		}
	}

	return true;
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	// 이미 스택 확인은 CanActivateAbility에서 했으므로 여기서 스택 소비
	if (m_AbilityID > 0)
	{
		// 스택 소비 시도
		// 참고: 직접 ConsumeAbilityStack을 호출하지 않고, 실제 로직에서 적절한 시점에 호출하도록 함
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// NET_LOG(LogTemp,Warning,TEXT("액티배이트 어빌리티"));

	// ToDO : 삭제 고민 Test
	m_ActorInfo = *ActorInfo;

	UAgentAbilitySystemComponent* asc = Cast<UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (asc)
	{
		if (FollowUpInputTags.IsEmpty() == false)
		{
			asc->SetSkillReady(false);
			asc->SetSkillClear(false);
			asc->ResisterFollowUpInput(FollowUpInputTags);
		}
		else
		{
			Active_General();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GA, asc가 AgentAbilitySystemComponent를 상속받지 않았어요."));
	}

	if (auto* ps = Cast<AAgentPlayerState>(ActorInfo->OwnerActor))
	{
		auto* agent = Cast<ABaseAgent>(ps->GetPawn());
		if (agent == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("BaseGameplayAbility, Agent Null"));
		}
        
		auto* curInteractor = agent->GetCurrentInterator();
		if (curInteractor)
		{
			NET_LOG(LogTemp,Warning,TEXT("인터랙터 숨기기 %s"), *curInteractor->GetActorNameOrLabel());
			curInteractor->SetActive(false);
			agent->ServerRPC_SetCurrentInteractor(nullptr);
		}
	}
}

void UBaseGameplayAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	NET_LOG(LogTemp, Warning, TEXT("스킬 InputPressed"));
}

void UBaseGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	NET_LOG(LogTemp, Warning, TEXT("스킬 InputReleased"));
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	NET_LOG(LogTemp, Warning, TEXT("스킬 EndAbility"));
	
	if (!bWasCancelled)
	{
		ConsumeAbilityStack(ActorInfo->PlayerController.Get());
	}

	ClearAgentSkill(ActorInfo);
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         bool bReplicateCancelAbility)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
	NET_LOG(LogTemp, Warning, TEXT("스킬 CancelAbility"));

	ClearAgentSkill(ActorInfo);
}

void UBaseGameplayAbility::Active_General()
{
}

void UBaseGameplayAbility::Active_Left_Click(FGameplayEventData data)
{
}

void UBaseGameplayAbility::Active_Right_Click(FGameplayEventData data)
{
}

void UBaseGameplayAbility::ClearAgentSkill(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (auto* ps = Cast<AAgentPlayerState>(ActorInfo->OwnerActor))
	{
		auto* asc = Cast<UAgentAbilitySystemComponent>(ps->GetAbilitySystemComponent());
		auto* agent = Cast<ABaseAgent>(ps->GetPawn());
        
		if (asc == nullptr || agent == nullptr)
		{
			UE_LOG(LogTemp,Error,TEXT("BaseGameplayAbility, ASC || Agent Null"));
			return;
		}

		asc->SetSkillClear(true);
		agent->SwitchInteractor(EInteractorType::MainWeapon);
	}
}
bool UBaseGameplayAbility::SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo)
{
	if (ProjectileClass == nullptr)
	{
		return false;
	}

	// Get the owning controller 
	AController* OwnerController = ActorInfo.PlayerController.Get();
	if (!OwnerController)
	{
		// Try to get controller from pawn if player controller is not available
		if (ActorInfo.OwnerActor.IsValid())
		{
			APawn* OwnerPawn = Cast<APawn>(ActorInfo.OwnerActor.Get());
			if (OwnerPawn)
			{
				OwnerController = OwnerPawn->GetController();
			}
		}

		if (!OwnerController)
		{
			return false;
		}
	}

	// Get the player controller for screen-to-world projection
	APlayerController* PlayerController = Cast<APlayerController>(OwnerController);
	if (!PlayerController)
	{
		return false;
	}

	// 카메라 매니저 구하기
	UGameplayStatics* GameplayStatics = nullptr;
	AActor* CameraActor = nullptr;
	FVector CameraLocation;
	FRotator CameraRotation;

	// 카메라 위치와 회전 구하기
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// 카메라 전방 벡터 가져오기 (화면 중앙이 바라보는 방향)
	FVector CameraForward = CameraRotation.Vector();

	// 발사 위치 계산 (카메라 위치 또는 폰 위치 + 약간의 오프셋)
	FVector SpawnLocation = CameraLocation;

	// 원하는 경우 특정 거리만큼 앞으로 이동
	// FVector SpawnLocation = CameraLocation + (CameraForward * 100.0f);

	// 발사 회전 (카메라 회전과 동일)
	FRotator SpawnRotation = CameraRotation;
	
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SpawnLocation);
	SpawnTransform.SetRotation(SpawnRotation.Quaternion());
	SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = Cast<APawn>(ActorInfo.OwnerActor.Get());
	SpawnParams.Owner = ActorInfo.OwnerActor.Get();
	
	AActor* SpawnedProjectile = GetWorld()->SpawnActor<AActor>(
		ProjectileClass,
		SpawnTransform,
		SpawnParams
	);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	return (SpawnedProjectile != nullptr);

	return false;
}

void UBaseGameplayAbility::SetAbilityID(int32 AbilityID)
{
	m_AbilityID = AbilityID;
}

// 어빌리티 스택 감소 메서드 구현
bool UBaseGameplayAbility::ConsumeAbilityStack(const APlayerController* PlayerController)
{
	// 플레이어 스테이트 가져오기
	AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
	if (PS == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 감소 실패: PlayerState가 NULL입니다."));
		return false;
	}

	// 현재 스택 가져오기
	int32 CurrentStack = PS->GetAbilityStack(m_AbilityID);

	// 스택이 있으면 감소
	if (CurrentStack > 0)
	{
		PS->ReduceAbilityStack(m_AbilityID);
		UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 감소됨. 남은 스택: %d"), m_AbilityID, CurrentStack - 1);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("어빌리티 ID %d의 스택이 없습니다."), m_AbilityID);
	return false;
}

// 어빌리티 스택 확인 메서드 구현
int32 UBaseGameplayAbility::GetAbilityStack(const APlayerController* PlayerController) const
{
	// 플레이어 스테이트 가져오기
	AAgentPlayerState* PS = Cast<AAgentPlayerState>(PlayerController->PlayerState);
	if (PS == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("어빌리티 스택 확인 실패: PlayerState가 NULL입니다."));
		return 0;
	}

	// 현재 스택 반환
	return PS->GetAbilityStack(m_AbilityID);
}
