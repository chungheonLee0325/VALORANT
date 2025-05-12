// NiagaraMeshCheckNotifyState.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "NiagaraSystem.h"
#include "Valorant/Player/Agent/BaseAgent.h" // BaseAgent 클래스를 사용하기 위해 추가
#include "NiagaraMeshCheckNotifyState.generated.h"

UCLASS(meta = (DisplayName = "Play Niagara Effect With Mesh Check"))
class VALORANT_API UNiagaraMeshCheckNotifyState : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    // Constructor
    UNiagaraMeshCheckNotifyState();

    // The Niagara system to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    UNiagaraSystem* NiagaraSystem;

    // Socket or bone to attach the effect to
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    FName SocketName;

    // Offset from the socket/bone
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    FVector LocationOffset;

    // Rotation offset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    FRotator RotationOffset;

    // Scale of the effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    FVector Scale;

    // Whether to attach the effect to the socket/bone
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    bool bAttached;
    
    // Only play when this is the first person mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    bool bFirstPersonOnly;

    // Only play when this is the third person mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
    bool bThirdPersonOnly;

    // Override functions from UAnimNotifyState
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
    virtual FString GetNotifyName_Implementation() const override;

private:
    // Store the spawned Niagara component for each mesh component
    TMap<USkeletalMeshComponent*, class UNiagaraComponent*> ActiveNiagaraComponents;
    
    // Helper function to check if this mesh is the one we want to use
    bool ShouldPlayEffect(USkeletalMeshComponent* MeshComp);
};