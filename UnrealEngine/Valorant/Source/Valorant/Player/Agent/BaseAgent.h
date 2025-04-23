#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "Player/AgentPlayerController.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "BaseAgent.generated.h"

class ABaseInteractor;
class AAgentPlayerState;
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

	UFUNCTION(BlueprintCallable)
	UAgentAbilitySystemComponent* GetASC() const { return ASC.Get(); }

	UFUNCTION(BlueprintCallable)
	int32 GetAgentID() const { return m_AgentID; }
	UFUNCTION(BlueprintCallable)
	void SetAgentID(const int32 id) { m_AgentID = id; }

	UFUNCTION(BlueprintCallable)
	void BindToDelegatePC(AAgentPlayerController* pc);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UCameraComponent* Camera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USpringArmComponent* SpringArm;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USkeletalMeshComponent* ThirdPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UAgentInputComponent* AgentInputComponent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UValorantGameInstance* m_GameInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AAgentPlayerState* PS = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AAgentPlayerController* PC = nullptr;
	
	TWeakObjectPtr<UAgentAbilitySystemComponent> ASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 m_AgentID = 0;
	
	FAgentData* m_AgentData = nullptr;

	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FindItemRange = 250.0f;

	UPROPERTY()
	ABaseInteractor* LookingActor = nullptr;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void InitAgentData();
	
	virtual void Die();
	virtual void EnterSpectMode();
	virtual void Respawn();

	void FindInteractable();
	
	UFUNCTION()
	void UpdateHealth(float newHealth);
	UFUNCTION()
	void UpdateMaxHealth(float newMaxHealth);
	UFUNCTION()
	void UpdateArmor(float newArmor);
	UFUNCTION()
	void UpdateMoveSpeed(float newSpeed);
};
