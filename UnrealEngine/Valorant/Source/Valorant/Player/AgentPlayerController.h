// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AgentPlayerController.generated.h"

class UAgentBaseWidget;
/**
 * 
 */
UCLASS()
class VALORANT_API AAgentPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Widget")
	TSubclassOf<UUserWidget> AgentWidgetClass;
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UValorantGameInstance* m_GameInstance;

	UPROPERTY(BlueprintReadWrite)
	UAgentBaseWidget* AgentWidget;
};
