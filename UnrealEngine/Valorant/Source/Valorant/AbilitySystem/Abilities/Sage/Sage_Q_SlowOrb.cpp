#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/BaseProjectile.h"

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
    
    // Get viewport size
    int32 ViewportSizeX;
    int32 ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    
    // Default to center of screen (modify as needed)
    FVector2D ScreenPosition(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
    
    // Deproject screen to world
    FVector WorldPosition;
    FVector WorldDirection;
    if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldPosition, WorldDirection))
    {
        // Create spawn transform
        FRotator SpawnRotation = WorldDirection.Rotation();
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(WorldPosition);
        SpawnTransform.SetRotation(SpawnRotation.Quaternion());
        SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
        
        // Spawn parameters
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.Instigator = Cast<APawn>(ActorInfo.OwnerActor.Get());
        SpawnParams.Owner = ActorInfo.OwnerActor.Get();
        
        // Spawn the projectile
        AActor* SpawnedProjectile = GetWorld()->SpawnActor<AActor>(
            ProjectileClass,
            SpawnTransform,
            SpawnParams
        );
        
        return (SpawnedProjectile != nullptr);
    }
    
    return false;
}

void USage_Q_SlowOrb::Active_Left_Click(FGameplayEventData data)
{
    SpawnProjectile(m_ActorInfo);
} 
