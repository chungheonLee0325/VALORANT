#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseAgent.generated.h"

UCLASS()
class VALORANT_API ABaseAgent : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseAgent();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

private:
	
};
