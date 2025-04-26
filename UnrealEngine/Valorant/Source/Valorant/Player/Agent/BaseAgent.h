#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "Player/AgentPlayerController.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "BaseAgent.generated.h"

class UTimelineComponent;
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
//시야 감지 상태 (적이보임, 적이보이지않음 , 마지막보인적의위치)
UENUM(BlueprintType)
enum class EAgentVisibility : uint8
{
	Visible,
	Hidden,
	LastKnown,
};


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
	void SetIsRun(const bool _bIsRun);
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Anim")
	UAnimMontage* AM_Die;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Crouch")
	UTimelineComponent* TL_Crouch;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Crouch")
	UCurveFloat* CrouchCurve;

	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

	// 에이전트의 현재 미니맵 가시성 상태
	UPROPERTY(Replicated,BlueprintReadWrite,Category="Minimap")
	EAgentVisibility MinimapVisibility;

	// 에이전트가 마지막으로 보인 시간 (네트워크 복제)
	UPROPERTY(Replicated)
	float LastVisibleTime;

	// 물음표 상태로 표시되는 시간 (에디터에서 변경 가능)
	UPROPERTY(EditAnywhere,Category="Minimap")
	float QuestionMarkDuration = 5.0f;

	// 팀 ID (적팀과 같은팀은 같은 아이디 사용하게 하자) - (네트워크 복제)
	UPROPERTY(Replicated,BlueprintReadWrite)
	int32 TeamID;
	
	// // 에이전트 아이콘 (에디터에서 변경 가능)
	// UPROPERTY(EditAnywhere,Category="Minimap")
	// UTexture2D* AgentIcon;
	//
	// // 물음표 아이콘 (에디터에서 변경 가능)
	// UPROPERTY(EditAnywhere,Category="Minimap")
	// UTexture2D* QuestionMarkIcon;

	// 다른 팀 플레이어에게 보이는지 체크
	bool IsVisibleToTeam(int32 ViewerTeamID) const;

	// 미니맵 시각화 상태를 업데이트
	void UpdateMinimapVisibility();

	// // 미니맵에 표시할 아이콘 가져오기 - (BP 호출)
	// UFUNCTION(BlueprintCallable, Category="Minimap")
	// UTexture2D* GetAgentIcon(int32 ViewerTeamID) const;
	//
	// // 적 플레이어 시야에 보이는지 체크하는 함수 - (BP 호출)
	// UFUNCTION(BlueprintCallable, Category="Vision")
	// bool IsVisibleToOpponents() const;

	// 네트워크 복제 속성 설정 - (언리얼 네트워크 이용)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	

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
	
	UPROPERTY(Replicated)
	bool bIsRun = true;
	UPROPERTY(Replicated)
	bool bIsDead = false;

	float BaseCapsuleHalfHeight = 0.0f;

	float BaseRunSpeed = 675.0f;
	float BaseWalkSpeed = 330.0f;
	
	float EquipSpeedMultiplier = 1.0f;
	float EffectSpeedMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FindItemRange = 250.0f;

	UPROPERTY()
	ABaseInteractor* LookingActor = nullptr;

public:

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable)
	void Server_SetIsRun(const bool _bIsRun);
	
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	UFUNCTION()
	void HandleCrouchProgress(float Value);
	
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


