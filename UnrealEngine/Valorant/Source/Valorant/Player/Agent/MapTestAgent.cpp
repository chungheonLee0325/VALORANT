// Fill out your copyright notice in the Description page of Project Settings.


#include "MapTestAgent.h"

#include "Kismet/GameplayStatics.h"


// Sets default values
AMapTestAgent::AMapTestAgent()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void AMapTestAgent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMapTestAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 매 틱마다 적감지 실행
	DetectAgents();
}

// Called to bind functionality to input
void AMapTestAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// 특정 위치가 시야에 있는지 
bool AMapTestAgent::IsPositionIsVision(const FVector& TargetPosition) const
{
	// 플레이아의 전방백터와 타켓 방향 백터 계산
	//FVector ForwardVector = GetActorForwardVector();
	//FVector DirectionToTarget =(TargetPosition - GetActorLocation().GetSafeNormal());

	// 두 백터 사이의 각도 계산 (내적)
	//float DotProduct = FVector::DotProduct(ForwardVector, DirectionToTarget);
	//float AngleCos = FMath::Cos(FMath::DegreesToRadians(VisionAngle * 0.5f));
	//시야 각도 제한
	// if (DotProduct > VisionAngle)
	// {
	// 	return false;
	// }
	
	// 무제한 시야거리 장애물에 가려지는지 체크 
	return !IsLineOfSightBlocked(nullptr);
}

// 다른 에이전트를 감지 확인
void AMapTestAgent::DetectAgents()
{
	// // 서버에서만 실행
	// if (GetLocalRole() != ROLE_Authority)
	// {
	// 	return;
	// }
	// // 월드에서 모든 BaseAgent 검색
	// TArray<AActor*> FoundActors;
	// UGameplayStatics::GetAllActorsOfClass(GetWorld(),ABaseAgent::StaticClass(),FoundActors);
	//
	// // 현재 게임 시간 얻기
	// float CurrentTime = UGameplayStatics::GetTimeSeconds(GetWorld());
	//
	// // 각 에이전트 검사
	// for (AActor* Actor : FoundActors)
	// {
	// 	// 자기자신 , 자기팀 일시 진행 
	// 	ABaseAgent* Agent = Cast<ABaseAgent>(Actor);
	// 	if (!Agent || Agent == this || Agent->TeamID == TeamID)
	// 	{
	// 		continue;
	// 	}
	// 	bool IsVisible = false;
	//
	// 	// 시야에 있고 장애물에 가려지지 않는지 확인
	// 	if (!IsLineOfSightBlocked(Agent))
	// 	{
	// 		IsVisible = true;
	//
	// 		// 적이 보이면 상태 업데이트
	// 		if (Agent->MinimapVisibility != EAgentVisibility::Visible)
	// 		{
	// 			Agent->MinimapVisibility = EAgentVisibility::Visible;
	// 		}
	//
	// 		Agent->LastVisibleTime = CurrentTime;
	// 	}
	// }
}

// 시야가 장애물에 차단되는지 확인
bool AMapTestAgent::IsLineOfSightBlocked(const AActor* TargetActor) const
{
	// 타켓이 없다면 시야가 차단된걸로 
	if (!TargetActor)
	{
		return true;
	}

	// 레이캐스트 시작점
	FVector StartTrace = GetActorLocation() + FVector(0, 0, 50);
	// 레이캐스트 종료지점
	FVector EndTrace = TargetActor->GetActorLocation() + FVector(0, 0, 50);

	// 레이캐스트 설정
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	// 자시자신 제외
	QueryParams.AddIgnoredActor(this);
	// 타켓 액터도 제외
	QueryParams.AddIgnoredActor(TargetActor);

	// 레이캐스트 실행 (두 지점 사이에 장애물이 있는지 확인)
	bool bHit = GetWorld()->LineTraceSingleByChannel
	(HitResult,StartTrace,EndTrace,ECC_Visibility,QueryParams);
	
	// 디버그용 레이 그리기 
	DrawDebugLine(GetWorld(), StartTrace, EndTrace, bHit ? FColor::Red : FColor::Green, false, 0.1f);

	// true면 장애물에 가려짐, false면 가려지지 않음
	return bHit;
}




