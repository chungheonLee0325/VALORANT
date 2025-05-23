#include "Sage_C_BarrierOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

USage_C_BarrierOrb::USage_C_BarrierOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);

	// === 후속 입력 설정 (CDO에서 안전한 방식) ===
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
	ValidFollowUpInputs.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));
	
	m_AbilityID = 1001;
	InputType = EAbilityInputType::MultiPhase;
	BarrierRotation = FRotator::ZeroRotator;
}

void USage_C_BarrierOrb::HandleLeftClick(FGameplayEventData EventData)
{
	Super::HandleLeftClick(EventData);

	// 장벽 생성 위치 계산 (플레이어 앞쪽)
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner || !BarrierClass) return;
	FVector SpawnLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * BarrierSpawnDistance;
	SpawnLoc.Z += BarrierSpawnHeight;
	FRotator SpawnRot = BarrierRotation;
	// 실제 장벽 스폰
	GetWorld()->SpawnActor<AActor>(BarrierClass, SpawnLoc, SpawnRot);
	TransitionToState(FValorantGameplayTags::Get().State_Ability_Executing);
	CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}

void USage_C_BarrierOrb::HandleRightClick(FGameplayEventData EventData)
{
	Super::HandleRightClick(EventData);

	// 장벽 회전값 변경
	BarrierRotation.Yaw += BarrierYawStep;
	
	// 디버그 박스 미리보기
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner) return;

	FVector SpawnLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * BarrierSpawnDistance;
	SpawnLoc.Z += BarrierSpawnHeight;
	FRotator SpawnRot = BarrierRotation;

	FVector BoxExtent(150.f, 50.f, 100.f); // UE는 반크기
	FQuat BoxQuat = SpawnRot.Quaternion();

	DrawDebugBox(
		GetWorld(),
		SpawnLoc,
		BoxExtent,
		BoxQuat,
		FColor::Green,
		false,      // bPersistentLines
		0.7f,       // LifeTime
		0,          // DepthPriority
		2.0f        // Thickness
	);
}
