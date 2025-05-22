#include "Sage_C_BarrierOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

USage_C_BarrierOrb::USage_C_BarrierOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);

	//FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.LeftClick")));
	//FollowUpInputTags.Add(FGameplayTag::RequestGameplayTag(FName("Input.Default.RightClick")));
	
	m_AbilityID = 1001;
	BarrierRotation = FRotator::ZeroRotator;
}

void USage_C_BarrierOrb::Active_Left_Click(FGameplayEventData data)
{
	// 장벽 생성 위치 계산 (플레이어 앞쪽)
	AActor* Owner = CachedActorInfo.AvatarActor.Get();
	if (!Owner || !BarrierClass) return;
	FVector SpawnLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * BarrierSpawnDistance;
	SpawnLoc.Z += BarrierSpawnHeight;
	FRotator SpawnRot = BarrierRotation;
	// 실제 장벽 스폰
	GetWorld()->SpawnActor<AActor>(BarrierClass, SpawnLoc, SpawnRot);
}

void USage_C_BarrierOrb::Active_Right_Click(FGameplayEventData data)
{
	// 장벽 회전값 변경
	BarrierRotation.Yaw += BarrierYawStep;
}
