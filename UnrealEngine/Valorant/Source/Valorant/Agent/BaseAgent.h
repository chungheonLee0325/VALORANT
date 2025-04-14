#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "BaseAgent.generated.h"

UCLASS()
class VALORANT_API ABaseAgent : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseAgent();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* ThirdPersonMesh;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	class UAgentAbilitySystemComponent* ASC;

	virtual void OnHealthChanged(const FOnAttributeChangeData& Data);
	virtual void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	virtual void OnArmorChanged(const FOnAttributeChangeData& Data);
	virtual void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

private:
	
};
