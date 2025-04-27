// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseAgent.h"
#include "MapTestAgent.generated.h"

UCLASS()
class VALORANT_API AMapTestAgent : public ABaseAgent
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMapTestAgent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 시야 각도
	UPROPERTY(EditAnywhere, Category= "Vision")
	float VisionAngle = 90.0f;

	// 특정 위치가 이 플레이어 시야에 있는지 확인 
	bool IsPositionIsVision(const FVector& TargetPosition) const;

	// 다른 에이전트를 감지하는 함수
	void DetectAgents();

	// 레이캐스트로 시야 차단 여부 확인
	bool IsLineOfSightBlocked(const AActor* TargetActor) const;
	
};
