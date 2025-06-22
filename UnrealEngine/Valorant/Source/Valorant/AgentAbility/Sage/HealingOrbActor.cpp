#include "HealingOrbActor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

AHealingOrbActor::AHealingOrbActor()
{
    // 베이스 설정 오버라이드
    OrbColor = NormalColor;
    OrbScale = 0.3f;
    OrbRotationSpeed = 90.0f;
    LightIntensity = 500.0f;
    LightRadius = 200.0f;
    EmissiveIntensity = 3.0f;
    
    // 펄스 애니메이션 사용
    AnimationType = EOrbAnimationType::Pulse;
    PulseSpeed = 2.0f;
    PulseScale = 0.1f;
    
    // 하이라이트 이펙트
    HighlightEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HighlightEffect"));
    HighlightEffect->SetupAttachment(OrbMesh);
    HighlightEffect->bAutoActivate = false;
    HighlightEffect->SetIsReplicated(true);
}

void AHealingOrbActor::BeginPlay()
{
    Super::BeginPlay();
    
    // 기본 색상을 NormalColor로 설정
    UpdateOrbColor(NormalColor);
}

void AHealingOrbActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AHealingOrbActor, bIsHighlighted);
}

void AHealingOrbActor::SetTargetHighlight(bool bHighlight)
{
    if (HasAuthority())
    {
        bIsHighlighted = bHighlight;
        OnRep_IsHighlighted();
    }
}

void AHealingOrbActor::OnRep_IsHighlighted()
{
    UpdateHighlightVisuals();
}

void AHealingOrbActor::UpdateHighlightVisuals()
{
    // 보이는 경우에만 하이라이트 업데이트
    if (!OrbMesh || !OrbMesh->IsVisible())
        return;
    
    // 하이라이트 이펙트 활성화/비활성화
    if (HighlightEffect)
    {
        if (bIsHighlighted)
        {
            HighlightEffect->Activate();
        }
        else
        {
            HighlightEffect->Deactivate();
        }
    }
    
    // 색상 변경
    FLinearColor TargetColor = bIsHighlighted ? HighlightColor : NormalColor;
    UpdateOrbColor(TargetColor);
    
    // 하이라이트 사운드 재생
    if (bIsHighlighted && OrbHighlightSound && IsValid(this))
    {
        // 로컬 플레이어에게만 재생 (베이스 클래스의 가시성 로직과 일치)
        APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
        if (LocalPC)
        {
            APawn* OwnerPawn = Cast<APawn>(GetOwner());
            bool bIsLocalOwner = LocalPC->GetPawn() == OwnerPawn;
            
            // 베이스 클래스에서 사용하는 것과 동일한 로직
            bool bShouldPlaySound = (GetOrbViewType() == EOrbViewType::FirstPerson && bIsLocalOwner) ||
                                   (GetOrbViewType() == EOrbViewType::ThirdPerson && !bIsLocalOwner);
            
            if (bShouldPlaySound)
            {
                UGameplayStatics::PlaySoundAtLocation(GetWorld(), OrbHighlightSound, GetActorLocation());
            }
        }
    }
}