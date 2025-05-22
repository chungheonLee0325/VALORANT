#include "Phoenix_Q_HotHands.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/AgentPlayerState.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AgentAbility/BaseProjectile.h"

UPhoenix_Q_HotHands::UPhoenix_Q_HotHands() : UBaseGameplayAbility()
{
	// === 어빌리티 기본 설정 ===
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 2002;
	InputType = EAbilityInputType::MultiPhase;

	// === 후속 입력 설정 (CDO에서 안전한 방식) ===
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));
}

void UPhoenix_Q_HotHands::HandleLeftClick(FGameplayEventData EventData)
{
	Super::HandleLeftClick(EventData);
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 좌클릭 처리 (직선 던지기)"));

	// 직선 던지기 실행
	ExecuteStraightThrow();
}

void UPhoenix_Q_HotHands::HandleRightClick(FGameplayEventData EventData)
{
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 우클릭 처리 (포물선 던지기)"));

	ExecuteCurvedThrow();
}

void UPhoenix_Q_HotHands::ExecuteStraightThrow()
{
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 던지기 실행"));

	// 직선 던지기 애니메이션 재생
	if (ThrowMontage && CachedActorInfo.GetAnimInstance())
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			"StraightThrowAnimation",
			ThrowMontage,
			1.0f
		);

		if (Task)
		{
			Task->OnCompleted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
			Task->OnInterrupted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
			Task->ReadyForActivation();
		}
	}

	// 직선 던지기 사운드 재생
	if (ThrowSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
	}

	// 핸드 이펙트 중지
	StopHandFireEffect();

	// 직선 투사체 생성
	if (SpawnProjectileByType(EPhoenixQThrowType::Straight))
	{
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 투사체 생성 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 직선 투사체 생성 실패"));
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UPhoenix_Q_HotHands::ExecuteCurvedThrow()
{
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 던지기 실행"));

	// 포물선 던지기 애니메이션 재생
	if (ThrowMontage && CachedActorInfo.GetAnimInstance())
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			"CurvedThrowAnimation",
			ThrowMontage,
			1.0f
		);

		if (Task)
		{
			Task->OnCompleted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
			Task->OnInterrupted.AddDynamic(this, &UPhoenix_Q_HotHands::OnThrowAnimationCompleted);
			Task->ReadyForActivation();
		}
	}

	// 포물선 던지기 사운드 재생
	if (ThrowSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
	}

	// 핸드 이펙트 중지
	StopHandFireEffect();

	// 포물선 투사체 생성
	if (SpawnProjectileByType(EPhoenixQThrowType::Curved))
	{
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 투사체 생성 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 포물선 투사체 생성 실패"));
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

bool UPhoenix_Q_HotHands::SpawnProjectileByType(EPhoenixQThrowType ThrowType)
{
	TSubclassOf<ABaseProjectile> ProjectileToSpawn = nullptr;

	switch (ThrowType)
	{
	case EPhoenixQThrowType::Straight:
		// ProjectileToSpawn = StraightProjectileClass ? StraightProjectileClass : ProjectileClass;
		break;
	case EPhoenixQThrowType::Curved:
		// ProjectileToSpawn = CurvedProjectileClass ? CurvedProjectileClass : ProjectileClass;
		break;
	default:
		ProjectileToSpawn = ProjectileClass;
		break;
	}
	ProjectileToSpawn = ProjectileClass;

	if (ProjectileToSpawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 투사체 클래스가 설정되지 않음"));
		return false;
	}

	// 기존 SpawnProjectile 로직을 사용하되, 투사체 클래스만 교체
	TSubclassOf<ABaseProjectile> OriginalProjectileClass = ProjectileClass;
	ProjectileClass = ProjectileToSpawn;

	bool bResult = SpawnProjectile(CachedActorInfo);

	// 원래 클래스로 복원
	ProjectileClass = OriginalProjectileClass;

	return bResult;
}

void UPhoenix_Q_HotHands::StartHandFireEffect()
{
	if (HandFireEffect && CachedActorInfo.OwnerActor.IsValid())
	{
		// 플레이어의 손 위치에 파티클 이펙트 생성
		if (ABaseAgent* Agent = Cast<ABaseAgent>(CachedActorInfo.OwnerActor.Get()))
		{
			// 적절한 소켓이나 위치에 파티클 컴포넌트 생성
			HandFireComponent = UGameplayStatics::SpawnEmitterAttached(
				HandFireEffect,
				Agent->GetMesh(),
				FName("hand_rSocket"), 
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
			);

			if (HandFireComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 핸드 파이어 이펙트 생성됨"));
			}
		}
	}
}

void UPhoenix_Q_HotHands::StopHandFireEffect()
{
	if (HandFireComponent && IsValid(HandFireComponent))
	{
		HandFireComponent->DestroyComponent();
		HandFireComponent = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 핸드 파이어 이펙트 중지됨"));
	}
}

void UPhoenix_Q_HotHands::OnThrowAnimationCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 발사 애니메이션 완료"));

	// 애니메이션 완료 후 추가 처리가 필요하다면 여기서
}

void UPhoenix_Q_HotHands::OnReadyEffectStarted()
{
	// 블루프린트에서 오버라이드 가능한 이벤트
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 준비 이펙트 시작 이벤트"));
}

void UPhoenix_Q_HotHands::CleanupAbility()
{
	UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 어빌리티 정리"));

	// 핸드 파이어 이펙트 정리
	StopHandFireEffect();
	
	// 던지기 타입 초기화
	CurrentThrowType = EPhoenixQThrowType::None;

	// 부모 클래스의 정리 작업 수행
	Super::CleanupAbility();
}

