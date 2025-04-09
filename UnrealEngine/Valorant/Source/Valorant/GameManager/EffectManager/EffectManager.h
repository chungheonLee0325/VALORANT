// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EffectManager.generated.h"

class UValorantGameInstance;
/**
 * 
 */
UCLASS()
class VALORANT_API UEffectManager : public UObject
{
	GENERATED_BODY()

public:
	void InitManager(UValorantGameInstance* GameInstance);

	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* Target, int EffectID, AActor* Instigator = nullptr);

private:
	UPROPERTY()
	UValorantGameInstance* m_GameInstance;
};
