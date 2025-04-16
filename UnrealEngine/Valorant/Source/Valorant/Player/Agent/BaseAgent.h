#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "BaseAgent.generated.h"

class UAgentBaseWidget;
class USpringArmComponent;
class UValorantGameInstance;
class UAgentInputComponent;
class UAgentAbilitySystemComponent;

UCLASS()
class VALORANT_API ABaseAgent : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseAgent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UCameraComponent* Camera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USpringArmComponent* SpringArm;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USkeletalMeshComponent* ThirdPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Widget")
	TSubclassOf<UUserWidget> AgentWidgetClass;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UValorantGameInstance* m_GameInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 m_AgentID = 0;

	FAgentData* m_AgentData = nullptr;

	UPROPERTY(BlueprintReadWrite)
	UAgentAbilitySystemComponent* ASC;

	UPROPERTY(BlueprintReadWrite)
	float RotOffset = -1.0f;

	UPROPERTY(BlueprintReadWrite)
	UAgentBaseWidget* AgentWidget;
	
public:
	UFUNCTION(BlueprintCallable)
	void AddCameraYawInput(float val);
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Crouch(bool bClientSimulation = false) override;
	
private:
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data);
	virtual void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	virtual void OnArmorChanged(const FOnAttributeChangeData& Data);
	virtual void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	
};
