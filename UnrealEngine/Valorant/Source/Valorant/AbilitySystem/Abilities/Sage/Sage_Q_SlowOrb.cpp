#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/BaseProjectile.h"
#include "Kismet/GameplayStatics.h"

USage_Q_SlowOrb::USage_Q_SlowOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));

	m_AbilityID = 1002;
}

bool USage_Q_SlowOrb::SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo)
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

void USage_Q_SlowOrb::Active_Left_Click(FGameplayEventData data)
{
	SpawnProjectile(m_ActorInfo);
}