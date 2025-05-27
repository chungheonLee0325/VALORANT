#include "Sage_C_BarrierOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

USage_C_BarrierOrb::USage_C_BarrierOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	
	m_AbilityID = 1001;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	BarrierRotation = FRotator::ZeroRotator;
}

bool USage_C_BarrierOrb::OnLeftClickInput()
{
	// 장벽 생성 위치 계산 (플레이어 앞쪽)
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner || !BarrierClass) return true;
	FVector SpawnLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * BarrierSpawnDistance;
	SpawnLoc.Z += BarrierSpawnHeight;
	FRotator SpawnRot = BarrierRotation;
	// 실제 장벽 스폰
	GetWorld()->SpawnActor<AActor>(BarrierClass, SpawnLoc, SpawnRot);
	return true;
}

bool USage_C_BarrierOrb::OnRightClickInput()
{
	// 장벽 회전값 변경
	BarrierRotation.Yaw += BarrierYawStep;
	
	// 디버그 박스 미리보기
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner) return false;

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
	return false;
}