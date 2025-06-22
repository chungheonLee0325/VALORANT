// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/FlashProjectile.h"
#include "Phoenix_E_P_Curveball.generated.h"

UCLASS()
class VALORANT_API APhoenix_E_P_Curveball : public AFlashProjectile
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APhoenix_E_P_Curveball();

	virtual void Tick(float DeltaTime) override;
	void SetCurveDirection(bool bCurveRight);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 곡선 관련 설정 - 발로란트 실제 수치
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float InitialSpeed = 1600.0f;
	
	// 곡선이 시작되기까지의 지연 시간
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float CurveDelay = 0.1f;  // 0.1초 후 곡선 시작

	// 최대 곡선 지속 시간
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float MaxCurveTime = 0.4f;

private:
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* CurveballMesh;
	
	// 상태 변수
	bool bShouldCurveRight = true;  // true: 오른쪽, false: 왼쪽
	bool bHasStartedCurving = false;
	float TimeSinceSpawn = 0.0f;

	// 회전 애니메이션 속도
	FRotator SpinRate = FRotator(0, 900, 0);  // Y축 회전

	// 곡선 강도 - 발로란트 실제 곡선 정도
	float CurveStrength = 5000.0f;  // 곡선의 강도 (더 급격한 곡선)
};