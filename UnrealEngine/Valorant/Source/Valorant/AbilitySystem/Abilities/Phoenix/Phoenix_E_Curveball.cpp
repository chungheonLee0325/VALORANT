#include "Phoenix_E_Curveball.h"

#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/FlashProjectile.h"
#include "AgentAbility/KayO/Flashbang.h"
#include "AgentAbility/Phoenix/Phoenix_E_EquippedCurveball.h"
#include "AgentAbility/Phoenix/Phoenix_E_P_Curveball.h"
#include "Player/Agent/BaseAgent.h"

UPhoenix_E_Curveball::UPhoenix_E_Curveball(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);

	m_AbilityID = 2003;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
}

bool UPhoenix_E_Curveball::OnLeftClickInput()
{
	bool bShouldExecute = true;
	DestroyEquippedCurveballs();
	
	// 섬광탄 발사
	SpawnFlashProjectile(false);

	return bShouldExecute;
}

bool UPhoenix_E_Curveball::OnRightClickInput()
{
	bool bShouldExecute = true;
	DestroyEquippedCurveballs();
	
	// 섬광탄 발사
	SpawnFlashProjectile(true);
	
	return bShouldExecute;
}

bool UPhoenix_E_Curveball::SpawnFlashProjectile(bool IsRight)
{
	if (!FlashProjectileClass)
		return false;
	
	// BaseGameplayAbility의 ProjectileClass 설정
	ProjectileClass = FlashProjectileClass;
    
	// 기본 SpawnProjectile 사용
	bool result = SpawnProjectile(FVector(50,0,50.f));
	if (auto flashBang = Cast<APhoenix_E_P_Curveball>(SpawnedProjectile))
	{
		flashBang->SetCurveDirection(IsRight);
	}
	return result;
}

void UPhoenix_E_Curveball::PrepareAbility()
{
	Super::PrepareAbility();
	SpawnEquippedCurveballs();
}

void UPhoenix_E_Curveball::SpawnEquippedCurveballs()
{
	if (!EquippedCurveballClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedCurveball3P = GetWorld()->SpawnActor<APhoenix_E_EquippedCurveball>(
			EquippedCurveballClass, HandLocation3P, HandRotation, SpawnParams3P);
		
		if (SpawnedCurveball3P)
		{
			SpawnedCurveball3P->SetCurveballViewType(EViewType::ThirdPerson);
			
			SpawnedCurveball3P->AttachToComponent(OwnerAgent->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
		}
	}
	
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;
		
		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedCurveball1P = GetWorld()->SpawnActor<APhoenix_E_EquippedCurveball>(
			EquippedCurveballClass, HandLocation1P, HandRotation, SpawnParams1P);
		
		if (SpawnedCurveball1P)
		{
			SpawnedCurveball1P->SetCurveballViewType(EViewType::FirstPerson);
			
			SpawnedCurveball1P->SetReplicates(false);
			
			SpawnedCurveball1P->AttachToComponent(OwnerAgent->GetMesh1P(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
		}
	}
}

void UPhoenix_E_Curveball::DestroyEquippedCurveballs()
{
	// 3인칭 수류탄 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedCurveball3P)
	{
		SpawnedCurveball3P->OnUnequip();
		SpawnedCurveball3P->Destroy();
		SpawnedCurveball3P = nullptr;
	}
	
	// 1인칭 수류탄 제거 (로컬에서만)
	if (SpawnedCurveball1P)
	{
		SpawnedCurveball1P->OnUnequip();
		SpawnedCurveball1P->Destroy();
		SpawnedCurveball1P = nullptr;
	}
}

void UPhoenix_E_Curveball::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	DestroyEquippedCurveballs();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
