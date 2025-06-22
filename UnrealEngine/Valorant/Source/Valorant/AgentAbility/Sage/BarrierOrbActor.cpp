#include "BarrierOrbActor.h"
#include "Net/UnrealNetwork.h"

ABarrierOrbActor::ABarrierOrbActor()
{
    // 베이스 설정 오버라이드
    OrbColor = FLinearColor(0.2f, 0.8f, 1.0f, 1.0f);  // 청록색
    OrbScale = 0.25f;
    OrbRotationSpeed = 60.0f;
    LightIntensity = 800.0f;
    LightRadius = 150.0f;
    
    // 부유 애니메이션 사용
    AnimationType = EOrbAnimationType::Float;
    FloatSpeed = 2.0f;
    FloatHeight = 5.0f;
}

void ABarrierOrbActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(ABarrierOrbActor, bIsPlacementValid);
}

void ABarrierOrbActor::SetPlacementValid(bool bValid)
{
    if (HasAuthority())
    {
        bIsPlacementValid = bValid;
        OnRep_IsPlacementValid();
    }
}

void ABarrierOrbActor::OnRep_IsPlacementValid()
{
    UpdatePlacementVisuals();
}

void ABarrierOrbActor::UpdatePlacementVisuals()
{
    // 보이는 경우에만 배치 상태 업데이트
    if (!OrbMesh || !OrbMesh->IsVisible())
        return;
    
    // 배치 가능 여부에 따라 색상 변경
    FLinearColor TargetColor = bIsPlacementValid ? OrbColor : FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);  // 빨간색
    
    // 베이스 클래스의 색상 업데이트 함수 사용
    UpdateOrbColor(TargetColor);
}