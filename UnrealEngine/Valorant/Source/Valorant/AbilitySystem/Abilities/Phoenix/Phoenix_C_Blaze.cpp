#include "Phoenix_C_Blaze.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/Phoenix/Phoenix_C_BlazeProjectile.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UPhoenix_C_Blaze::UPhoenix_C_Blaze(): UBaseGameplayAbility()
{
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
    SetAssetTags(Tags);

    m_AbilityID = 2001;
    
    // Blaze는 준비 단계가 있음
    ActivationType = EAbilityActivationType::WithPrepare;
    FollowUpInputType = EFollowUpInputType::LeftOrRight;
}

bool UPhoenix_C_Blaze::OnLeftClickInput()
{
    bool bShouldExecute = true;
    
    SpawnBlazeProjectile(EBlazeMovementType::Straight);
    
    return bShouldExecute;
}

bool UPhoenix_C_Blaze::OnRightClickInput()
{
    bool bShouldExecute = true;
    
    SpawnBlazeProjectile(EBlazeMovementType::Curved);
    
    return bShouldExecute;
}

void UPhoenix_C_Blaze::ExecuteAbility()
{
    // 벽 투사체가 이미 스폰되었으므로 추가 작업 없음
    UE_LOG(LogTemp, Warning, TEXT("Phoenix C - Blaze 실행 완료"));
}

bool UPhoenix_C_Blaze::SpawnBlazeProjectile(EBlazeMovementType MovementType)
{
    if (!HasAuthority(&CurrentActivationInfo) || !BlazeProjectileClass)
    {
        return false;
    }
    
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        return false;
    }
    
    // 발사 위치 계산 (캐릭터 발 근처)
    FVector SpawnLocation = Character->GetActorLocation();
    FRotator SpawnRotation = Character->GetControlRotation();
    SpawnRotation.Pitch = 0;  // 수평으로만 발사
    
    // 전방으로 약간 오프셋 (캐릭터 앞쪽)
    SpawnLocation += SpawnRotation.Vector() * 50.0f;
    SpawnLocation.Z -= 50.0f;  // 발 높이로 조정
    
    // 지면 체크
    FHitResult GroundHit;
    FVector TraceStart = SpawnLocation + FVector(0, 0, 50);
    FVector TraceEnd = SpawnLocation - FVector(0, 0, 200);
    
    if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, 
        ECollisionChannel::ECC_WorldStatic))
    {
        //DrawDebugSphere(GetWorld(),SpawnLocation,20,20,FColor::Red,false, 1.f);
        SpawnLocation = GroundHit.Location + FVector(0, 0, 10);  // 지면에서 약간 위
        //SpawnLocation = GroundHit.Location + FVector(0, 0, 100);  // 지면에서 약간 위
       // DrawDebugSphere(GetWorld(),SpawnLocation,20,20,FColor::Green,false, 1.f);
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;
    SpawnParams.Instigator = Character;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    APhoenix_C_BlazeProjectile* BlazeProjectile = GetWorld()->SpawnActor<APhoenix_C_BlazeProjectile>(BlazeProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (BlazeProjectile)
    {
        // 이동 타입 설정
        BlazeProjectile->SetMovementType(MovementType);
        
        UE_LOG(LogTemp, Warning, TEXT("Phoenix C - Blaze 투사체 생성 성공 (Type: %s)"), 
            MovementType == EBlazeMovementType::Straight ? TEXT("Straight") : TEXT("Curved"));
        
        return true;
    }
    
    return false;
}