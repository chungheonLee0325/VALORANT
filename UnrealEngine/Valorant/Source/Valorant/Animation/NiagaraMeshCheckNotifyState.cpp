// NiagaraMeshCheckNotifyState.cpp
#include "NiagaraMeshCheckNotifyState.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/Animaiton/AgentAnimInstance.h" // 필요한 애니메이션 인스턴스 클래스 추가

UNiagaraMeshCheckNotifyState::UNiagaraMeshCheckNotifyState()
{
    // Default values
    NiagaraSystem = nullptr;
    SocketName = NAME_None;
    LocationOffset = FVector::ZeroVector;
    RotationOffset = FRotator::ZeroRotator;
    Scale = FVector(1.0f);
    bAttached = true;
    bFirstPersonOnly = false;
    bThirdPersonOnly = false;
}

bool UNiagaraMeshCheckNotifyState::ShouldPlayEffect(USkeletalMeshComponent* MeshComp)
{
    if (!MeshComp)
    {
        return false;
    }
    
    // 메쉬 컴포넌트의 소유자인 Actor를 BaseAgent로 캐스팅
    ABaseAgent* OwningAgent = Cast<ABaseAgent>(MeshComp->GetOwner());
    if (!OwningAgent)
    {
        return false;
    }
    
    // 1인칭 메쉬인지 확인
    bool bIsFirstPersonMesh = (MeshComp == OwningAgent->GetMesh1P());
    // 3인칭 메쉬인지 확인
    bool bIsThirdPersonMesh = (MeshComp == OwningAgent->GetMesh());
    
    // 1인칭 전용이고 현재 메쉬가 1인칭 메쉬인 경우
    if (bFirstPersonOnly && bIsFirstPersonMesh)
    {
        return true;
    }
    
    // 3인칭 전용이고 현재 메쉬가 3인칭 메쉬인 경우
    if (bThirdPersonOnly && bIsThirdPersonMesh)
    {
        return true;
    }
    
    // 특정 메쉬 조건이 없는 경우(둘 다 false) 항상 재생
    if (!bFirstPersonOnly && !bThirdPersonOnly)
    {
        return true;
    }
    
    // 조건에 맞지 않으면 재생하지 않음
    return false;
}

void UNiagaraMeshCheckNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
    // 현재 메쉬가 재생 조건에 맞는지 확인
    if (!ShouldPlayEffect(MeshComp))
    {
        return;
    }
    
    // Only proceed if we have a valid mesh component and Niagara system
    if (!MeshComp || !NiagaraSystem)
    {
        return;
    }

    // Get the world context
    UWorld* World = MeshComp->GetWorld();
    if (!World)
    {
        return;
    }

    // Create the Niagara component
    UNiagaraComponent* NiagaraComponent = nullptr;

    if (bAttached)
    {
        // Spawn attached Niagara component
        NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            NiagaraSystem,
            MeshComp,
            SocketName,
            LocationOffset,
            RotationOffset,
            EAttachLocation::KeepRelativeOffset,
            true
        );
    }
    else
    {
        // Get the socket transform
        FTransform SocketTransform = MeshComp->GetSocketTransform(SocketName);
        
        // Apply offset
        FVector Location = SocketTransform.GetLocation() + SocketTransform.TransformVector(LocationOffset);
        FRotator Rotation = SocketTransform.Rotator() + RotationOffset;

        // Spawn at location
        NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            NiagaraSystem,
            Location,
            Rotation,
            Scale,
            true,
            true
        );
    }

    // Store the component for cleanup
    if (NiagaraComponent)
    {
        ActiveNiagaraComponents.Add(MeshComp, NiagaraComponent);
        
        // Apply scale if attached (since SpawnSystemAttached doesn't take scale)
        if (bAttached)
        {
            NiagaraComponent->SetRelativeScale3D(Scale);
        }
    }
}

void UNiagaraMeshCheckNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    // Find the Niagara component for this mesh
    UNiagaraComponent** FoundComponent = ActiveNiagaraComponents.Find(MeshComp);
    
    if (FoundComponent && *FoundComponent)
    {
        // Deactivate the system but keep it alive to let particles finish
        (*FoundComponent)->Deactivate();
        
        // Set it to auto destroy so it cleans itself up when done
        (*FoundComponent)->SetAutoDestroy(true);
        
        // Remove from our map
        ActiveNiagaraComponents.Remove(MeshComp);
    }
}

FString UNiagaraMeshCheckNotifyState::GetNotifyName_Implementation() const
{
    FString MeshTypeStr = bFirstPersonOnly ? TEXT("1P Only") : (bThirdPersonOnly ? TEXT("3P Only") : TEXT("All Meshes"));
    return NiagaraSystem ? FString::Printf(TEXT("Niagara: %s (%s)"), *NiagaraSystem->GetName(), *MeshTypeStr) : TEXT("Niagara Effect");
}