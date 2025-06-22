#include "BarrierWallActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

ABarrierWallActor::ABarrierWallActor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // 루트 컴포넌트
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);
    
    // 세그먼트 1 (왼쪽에서 첫 번째)
    SegmentMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SegmentMesh1"));
    SegmentMesh1->SetupAttachment(RootSceneComponent);
    
    SegmentCollision1 = CreateDefaultSubobject<UBoxComponent>(TEXT("SegmentCollision1"));
    SegmentCollision1->SetupAttachment(SegmentMesh1);
    
    // 세그먼트 2 (왼쪽에서 두 번째)
    SegmentMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SegmentMesh2"));
    SegmentMesh2->SetupAttachment(RootSceneComponent);
    
    SegmentCollision2 = CreateDefaultSubobject<UBoxComponent>(TEXT("SegmentCollision2"));
    SegmentCollision2->SetupAttachment(SegmentMesh2);
    
    // 세그먼트 3 (왼쪽에서 세 번째)
    SegmentMesh3 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SegmentMesh3"));
    SegmentMesh3->SetupAttachment(RootSceneComponent);
    
    SegmentCollision3 = CreateDefaultSubobject<UBoxComponent>(TEXT("SegmentCollision3"));
    SegmentCollision3->SetupAttachment(SegmentMesh3);
    
    // 세그먼트 4 (가장 오른쪽)
    SegmentMesh4 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SegmentMesh4"));
    SegmentMesh4->SetupAttachment(RootSceneComponent);
    
    SegmentCollision4 = CreateDefaultSubobject<UBoxComponent>(TEXT("SegmentCollision4"));
    SegmentCollision4->SetupAttachment(SegmentMesh4);
    
    // 네트워크 설정
    bReplicates = true;
    SetReplicateMovement(true);
}

void ABarrierWallActor::SetPlacementValid(bool bValid)
{
    // 미리보기 모드에서는 항상 유효하게 표시
    // 인터페이스 호환성을 위해 함수는 유지
}

void ABarrierWallActor::SetOnlyOwnerSee(bool bCond)
{
    SegmentMesh1->SetOnlyOwnerSee(bCond);
    SegmentMesh2->SetOnlyOwnerSee(bCond);
    SegmentMesh3->SetOnlyOwnerSee(bCond);
    SegmentMesh4->SetOnlyOwnerSee(bCond);
}

void ABarrierWallActor::BeginPlay()
{
    Super::BeginPlay();
    
    // 컴포넌트 배열 초기화
    SegmentMeshes.Add(SegmentMesh1);
    SegmentMeshes.Add(SegmentMesh2);
    SegmentMeshes.Add(SegmentMesh3);
    SegmentMeshes.Add(SegmentMesh4);
    
    SegmentCollisions.Add(SegmentCollision1);
    SegmentCollisions.Add(SegmentCollision2);
    SegmentCollisions.Add(SegmentCollision3);
    SegmentCollisions.Add(SegmentCollision4);
    
    // 체력 배열 초기화
    SegmentHealthArray.Init(DefaultSegmentHealth, 4);
    SegmentDestroyedArray.Init(false, 4);
    
    // 메쉬 크기만 설정 (위치는 나중에 설정)
    for (int32 i = 0; i < SegmentMeshes.Num(); i++)
    {
        if (SegmentMeshes[i] && SegmentMeshes[i]->GetStaticMesh())
        {
            FVector MeshBounds = SegmentMeshes[i]->GetStaticMesh()->GetBounds().BoxExtent * 2;
            FVector TargetSize(SegmentWidth, SegmentThickness, SegmentHeight);
            FVector Scale = TargetSize / MeshBounds;
            SegmentMeshes[i]->SetRelativeScale3D(Scale);
        }
        
        // 충돌 박스 크기 설정
        if (SegmentCollisions[i])
        {
            SegmentCollisions[i]->SetBoxExtent(FVector(SegmentWidth / 2, SegmentThickness / 2, SegmentHeight / 2));
        }
    }
    
    // 미리보기 모드가 아닐 때만 충돌 활성화 및 건설 시작
    if (!bIsPreviewMode)
    {
        // 충돌 설정
        for (UBoxComponent* Collision : SegmentCollisions)
        {
            if (Collision)
            {
                Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                Collision->SetCollisionResponseToAllChannels(ECR_Block);
                // 발로란트처럼 플레이어는 통과 가능
                Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
            }
        }
        
        // 건설 애니메이션 시작
        StartBuildAnimation();
    }
    else
    {
        // 미리보기 모드에서는 반투명 머티리얼 적용하고 최종 위치에 배치
        if (PreviewMaterial)
        {
            for (UStaticMeshComponent* Mesh : SegmentMeshes)
            {
                if (Mesh)
                {
                    Mesh->SetMaterial(0, PreviewMaterial);
                }
            }
        }
        
        // 미리보기는 최종 위치에 바로 표시
        SetSegmentsFinalPosition();
    }
}

void ABarrierWallActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bIsBuilding && !bIsPreviewMode)
    {
        BuildProgress = FMath::Min(BuildProgress + DeltaTime / BuildDuration, 1.0f);
        
        // 건설 애니메이션 - 아래에서 위로 올라오는 효과
        for (int32 i = 0; i < SegmentMeshes.Num(); i++)
        {
            if (SegmentMeshes[i])
            {
                // 각 세그먼트를 약간의 지연을 두고 올라오게 함
                float SegmentDelay = 0.1f * i;
                float SegmentProgress = FMath::Clamp((BuildProgress - SegmentDelay) / (1.0f - SegmentDelay * 3), 0.0f, 1.0f);
                
                // 위치 애니메이션 (아래에서 위로)
                float ZOffset = SegmentHeight / 2 * SegmentProgress;
                float XPos = GetSegmentXPosition(i);
                SegmentMeshes[i]->SetRelativeLocation(FVector(XPos, 0, ZOffset));
                
                // 스케일 애니메이션 (Z축만)
                if (SegmentMeshes[i]->GetStaticMesh())
                {
                    FVector MeshBounds = SegmentMeshes[i]->GetStaticMesh()->GetBounds().BoxExtent * 2;
                    FVector TargetSize(SegmentWidth, SegmentThickness, SegmentHeight);
                    FVector BaseScale = TargetSize / MeshBounds;
                    BaseScale.Z *= SegmentProgress;
                    SegmentMeshes[i]->SetRelativeScale3D(BaseScale);
                }
            }
        }
        
        if (BuildProgress >= 1.0f)
        {
            CompleteBuild();
        }
    }
}

void ABarrierWallActor::InitializeBarrier(float SegmentHealth, float Lifespan, float BuildTime)
{
    DefaultSegmentHealth = SegmentHealth;
    DefaultLifespan = Lifespan;
    BuildDuration = BuildTime;
    
    // 체력 초기화
    for (int32 i = 0; i < SegmentHealthArray.Num(); i++)
    {
        SegmentHealthArray[i] = SegmentHealth;
    }
    
    // 수명 타이머
    if (Lifespan > 0.f && !bIsPreviewMode)
    {
        GetWorld()->GetTimerManager().SetTimer(LifespanTimerHandle, this, 
            &ABarrierWallActor::DestroyBarrier, Lifespan, false);
    }
}

void ABarrierWallActor::SetPreviewMode(bool bPreview)
{
    bIsPreviewMode = bPreview;
    
    if (bIsPreviewMode)
    {
        SetActorEnableCollision(false);
        bIsBuilding = false;
        BuildProgress = 1.0f;
        SetReplicates(false);
    }
}

void ABarrierWallActor::StartBuildAnimation()
{
    bIsBuilding = true;
    BuildProgress = 0.0f;
    
    // 초기 위치 설정 (지면 아래)
    for (int32 i = 0; i < SegmentMeshes.Num(); i++)
    {
        if (SegmentMeshes[i])
        {
            float XPos = GetSegmentXPosition(i);
            SegmentMeshes[i]->SetRelativeLocation(FVector(XPos, 0, 0));
            
            // 초기 스케일 설정 (Z축을 0으로)
            if (SegmentMeshes[i]->GetStaticMesh())
            {
                FVector MeshBounds = SegmentMeshes[i]->GetStaticMesh()->GetBounds().BoxExtent * 2;
                FVector TargetSize(SegmentWidth, SegmentThickness, SegmentHeight);
                FVector InitialScale = TargetSize / MeshBounds;
                InitialScale.Z = 0.01f;
                SegmentMeshes[i]->SetRelativeScale3D(InitialScale);
            }
        }
    }
    
    // 건설 이펙트
    if (BuildEffect)
    {
        for (UStaticMeshComponent* Mesh : SegmentMeshes)
        {
            if (Mesh)
            {
                UNiagaraFunctionLibrary::SpawnSystemAttached(
                    BuildEffect,
                    Mesh,
                    NAME_None,
                    FVector(0, 0, -SegmentHeight / 2),
                    FRotator::ZeroRotator,
                    EAttachLocation::KeepRelativeOffset,
                    true
                );
            }
        }
    }
    
    // 건설 사운드 (한 번만 재생)
    if (BuildSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), BuildSound, GetActorLocation());
    }
}

void ABarrierWallActor::CompleteBuild()
{
    bIsBuilding = false;
    
    // 최종 위치 설정
    SetSegmentsFinalPosition();
    
    // 최종 머티리얼 적용
    if (BarrierMaterial)
    {
        for (UStaticMeshComponent* Mesh : SegmentMeshes)
        {
            if (Mesh)
            {
                Mesh->SetMaterial(0, BarrierMaterial);
            }
        }
    }
}

void ABarrierWallActor::SetSegmentsFinalPosition()
{
    for (int32 i = 0; i < SegmentMeshes.Num(); i++)
    {
        if (SegmentMeshes[i])
        {
            float XPos = GetSegmentXPosition(i);
            SegmentMeshes[i]->SetRelativeLocation(FVector(XPos, 0, SegmentHeight / 2));
            
            // 최종 스케일 확인
            if (SegmentMeshes[i]->GetStaticMesh())
            {
                FVector MeshBounds = SegmentMeshes[i]->GetStaticMesh()->GetBounds().BoxExtent * 2;
                FVector TargetSize(SegmentWidth, SegmentThickness, SegmentHeight);
                FVector FinalScale = TargetSize / MeshBounds;
                SegmentMeshes[i]->SetRelativeScale3D(FinalScale);
            }
        }
    }
}

float ABarrierWallActor::GetSegmentXPosition(int32 SegmentIndex) const
{
    // 일관된 세그먼트 위치 계산
    float TotalWidth = SegmentWidth * 4;
    float StartX = -TotalWidth / 2 + SegmentWidth / 2;
    return StartX + (SegmentIndex * SegmentWidth);
}

float ABarrierWallActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
                                    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsBuilding || bIsPreviewMode)
        return 0.f;

    FVector HitLocation = FVector(0, 0, 0);
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        FPointDamageEvent* const pointDamageEvent = (FPointDamageEvent*)&DamageEvent;
        HitLocation = pointDamageEvent->HitInfo.Location;
    }
    // 데미지 위치에서 가장 가까운 세그먼트 찾기
    FVector DamageLocation = DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation();
    if (HitLocation != FVector(0, 0, 0)) DamageLocation = HitLocation;
    int32 ClosestSegmentIndex = -1;
    float ClosestDistance = FLT_MAX;
    
    for (int32 i = 0; i < SegmentMeshes.Num(); i++)
    {
        if (!SegmentDestroyedArray[i] && SegmentMeshes[i])
        {
            float Distance = FVector::Dist(DamageLocation, SegmentMeshes[i]->GetComponentLocation());
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestSegmentIndex = i;
            }
        }
    }
    
    if (ClosestSegmentIndex >= 0)
    {
        SegmentHealthArray[ClosestSegmentIndex] -= DamageAmount;
        
        // 데미지 이펙트
        if (DamageEffect && SegmentMeshes[ClosestSegmentIndex])
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), 
                DamageEffect, 
                SegmentMeshes[ClosestSegmentIndex]->GetComponentLocation()
            );
        }
        
        // 데미지 사운드
        if (DamageSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), DamageSound, 
                SegmentMeshes[ClosestSegmentIndex]->GetComponentLocation());
        }
        
        // 손상된 머티리얼 적용
        if (SegmentHealthArray[ClosestSegmentIndex] <= DefaultSegmentHealth * 0.5f && BarrierDamagedMaterial)
        {
            SegmentMeshes[ClosestSegmentIndex]->SetMaterial(0, BarrierDamagedMaterial);
        }
        
        // 세그먼트 파괴
        if (SegmentHealthArray[ClosestSegmentIndex] <= 0.f)
        {
            NetMulti_DestroySegment(ClosestSegmentIndex);
        }
    }
    
    return DamageAmount;
}

void ABarrierWallActor::NetMulti_DestroySegment_Implementation(int32 SegmentIndex)
{
    if (SegmentIndex < 0 || SegmentIndex >= 4 || SegmentDestroyedArray[SegmentIndex])
        return;
    
    SegmentDestroyedArray[SegmentIndex] = true;
    
    // 파괴 이펙트
    if (DestroyEffect && SegmentMeshes[SegmentIndex])
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), 
            DestroyEffect, 
            SegmentMeshes[SegmentIndex]->GetComponentLocation()
        );
    }
    
    // 파괴 사운드
    if (DestroySound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), DestroySound, 
            SegmentMeshes[SegmentIndex]->GetComponentLocation());
    }
    
    // 세그먼트 숨기기
    if (SegmentMeshes[SegmentIndex])
    {
        SegmentMeshes[SegmentIndex]->SetVisibility(false);
        SegmentMeshes[SegmentIndex]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    if (SegmentCollisions[SegmentIndex])
    {
        SegmentCollisions[SegmentIndex]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // 모든 세그먼트 파괴 확인
    bool bAllDestroyed = true;
    for (bool bDestroyed : SegmentDestroyedArray)
    {
        if (!bDestroyed)
        {
            bAllDestroyed = false;
            break;
        }
    }
    
    if (bAllDestroyed)
    {
        DestroyBarrier();
    }
}

void ABarrierWallActor::DestroyBarrier()
{
    GetWorld()->GetTimerManager().ClearTimer(BuildTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(LifespanTimerHandle);
    
    Destroy();
}