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

//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//             CYT             ♣
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
// 시야 감지 상태 (적이보임, 적이보이지않음 , 마지막보인적의위치
// UENUM(BlueprintType)
// enum class EEnemyVisibility : uint8
// {
// 	NotVisible,
// 	Visible,
// 	LastKnownPosition,
// };


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
	bool GetIsRun() const { return  bIsRun; }
	UFUNCTION(BlueprintCallable)
	void SetIsRun(const bool _bIsRun) { bIsRun = _bIsRun; }
	
	UFUNCTION(BlueprintCallable)
	float GetEffectSpeedMulitiplier() const { return EffectSpeedMultiplier; }
	UFUNCTION(BlueprintCallable)
	void SetEffectSpeedMultiplier(const float newEffectSpeed) { EffectSpeedMultiplier = newEffectSpeed; }
	UFUNCTION(BlueprintCallable)
	float GetEquipSpeedMuliplier() const { return EquipSpeedMultiplier; }
	UFUNCTION(BlueprintCallable)
	void SetEquipSpeedMultiplier(const float newEquipSpeed) { EquipSpeedMultiplier = newEquipSpeed; }

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


	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	// 미니맵의 위치를 가져오는 함수
	// UFUNCTION(BlueprintCallable,Category="Minimap")
	// FVector2D GetMinimapPosition() const;
 //
	// // 적 시야 상태 변경 함수
	// UFUNCTION(BlueprintCallable,Category="Minimap")
	// void SetEnemyVisibility(ABaseAgent* Enemy,EEnemyVisibility NewVisibility);
	//
	// // 미니맵에 표시될 캐릭터아이콘
 //    UPROPERTY(EditDefaultsOnly,Category= "Minimap")
	// UTexture2D* MinimapIcon;
 //
	// // 마지막으로 본 위치 물음표아이콘
 //    UPROPERTY(EditDefaultsOnly,Category= "Minimap")
	// UTexture2D* LastKnownPositionIcon;
 //
	// // 물음표 지속시간
	// UPROPERTY(EditDefaultsOnly,Category= "Vision")
	// float LastKnownPositionDuration = 5.0f;
 //
	// //시야 체크 주기
	// UPROPERTY(EditDefaultsOnly,Category= "Vision")
	// float VisionCheckFrequency = 0.1f;
	//
	// // 현재 시야에 있는 적들 추적
	// UPROPERTY(Replicated)
	// TArray<ABaseAgent*> VisibleEnemies;
 //
	// // 마지막으로 본 적들 (물음표 표시)
	// //UPROPERTY(Replicated)
	// //TMap<ABaseAgent*,float> LastSeenEnemies;
 //
	// // 시야 체크 타이머 핸들
	// FTimerHandle VisionCheckTimerHandle;
 //
	// // 적 시야 체크 함수
	// void CheckEnemiesVisibility();
 //
	// // 적이 시야에 있는지 확인하는 함수
	// bool IsEnemyVisible(ABaseAgent* Enemy) const;
 //
	// // 네트워크 복제 설정
	// virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

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

	bool bIsRun = true;
	bool bIsDead = false;

	float BaseRunSpeed = 675.0f;
	float BaseWalkSpeed = 330.0f;
	
	float EquipSpeedMultiplier = 1.0f;
	float EffectSpeedMultiplier = 1.0f;
	
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
	
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	
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
	void UpdateEffectSpeed(float newEffectSpeed);
};
