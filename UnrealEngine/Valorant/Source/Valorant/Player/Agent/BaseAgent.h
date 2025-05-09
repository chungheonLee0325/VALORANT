#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Player/AgentPlayerController.h"
#include "Player/Animaiton/AgentAnimInstance.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "Weapon/BaseWeapon.h"
#include "BaseAgent.generated.h"

class UAgentAnimInstance;
class ABaseWeapon;
class ASpike;
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

UENUM(BlueprintType)
enum class EAgentDamagedPart : uint8
{
	None,
	Head,
	Body,
	Legs
};

UCLASS()
class VALORANT_API ABaseAgent : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseAgent();

	static EAgentDamagedPart GetHitDamagedPart(const FName& BoneName);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UCameraComponent* Camera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USpringArmComponent* SpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USkeletalMeshComponent* ThirdPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UAgentInputComponent* AgentInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* InteractionCapsule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Anim")
	UAnimMontage* AM_Die;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Anim")
	UAnimMontage* AM_Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Crouch")
	UTimelineComponent* TL_Crouch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Crouch")
	UCurveFloat* CrouchCurve;

	UPROPERTY(EditAnywhere, Category= "Die")
	float DieCameraTimeRange = 3.0f;;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Die")
	UTimelineComponent* TL_DieCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Die")
	UCurveFloat* DieCameraPitchCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Die")
	UCurveVector* DieCameraCurve;

	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

	// 에이전트의 현재 미니맵 가시성 상태
	UPROPERTY(Replicated, BlueprintReadWrite, Category="Minimap")
	EAgentVisibility MinimapVisibility;

	// 에이전트가 마지막으로 보인 시간 (네트워크 복제)
	UPROPERTY(Replicated)
	float LastVisibleTime;

	// 물음표 상태로 표시되는 시간 (에디터에서 변경 가능)
	UPROPERTY(EditAnywhere, Category="Minimap")
	float QuestionMarkDuration = 5.0f;

	// 팀 ID (적팀과 같은팀은 같은 아이디 사용하게 하자) - (네트워크 복제)
	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 TeamID;

	// // 에이전트 아이콘 (에디터에서 변경 가능)
	UPROPERTY(EditAnywhere, Category="Minimap")
	UTexture2D* AgentIcon;

	// // 물음표 아이콘 (에디터에서 변경 가능)
	UPROPERTY(EditAnywhere, Category="Minimap")
	UTexture2D* QuestionMarkIcon;

	// 다른 팀 플레이어에게 보이는지 체크
	bool IsVisibleToTeam(int32 ViewerTeamID) const;

	// 미니맵 시각화 상태를 업데이트
	void UpdateMinimapVisibility();

	// // 미니맵에 표시할 아이콘 가져오기 - (BP 호출)
	UFUNCTION(BlueprintCallable, Category="Minimap")
	UTexture2D* GetAgentIcon(int32 ViewerTeamID) const;

	// 적 플레이어 시야에 보이는지 체크하는 함수 - (BP 호출)
	UFUNCTION(BlueprintCallable, Category="Vision")
	bool IsVisibleToOpponents() const;

	// 네트워크 복제 속성 설정 - (언리얼 네트워크 이용)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void BindToDelegatePC(AAgentPlayerController* pc);

	UFUNCTION(BlueprintCallable)
	UAgentAbilitySystemComponent* GetASC() const { return ASC.Get(); }

	UFUNCTION(BlueprintCallable)
	int32 GetAgentID() const { return m_AgentID; }

	UFUNCTION(BlueprintCallable)
	void SetAgentID(const int32 id) { m_AgentID = id; }

	UFUNCTION(Server, Reliable)
	void ServerApplyGE(TSubclassOf<UGameplayEffect> geClass);
	UFUNCTION(Server, Reliable)
	void ServerApplyHitScanGE(TSubclassOf<UGameplayEffect> geClass, const int Damage);

	UFUNCTION(BlueprintCallable)
	bool GetIsRun() const { return bIsRun; }

	UFUNCTION(BlueprintCallable)
	void SetIsRun(const bool _bIsRun);
	UFUNCTION(Server, Reliable)
	void Server_SetIsRun(const bool _bIsRun);

	UFUNCTION(BlueprintCallable)
	EInteractorType GetInteractorState() const { return CurrentInteractorState; }

	ABaseWeapon* GetMainWeapon() const;
	ABaseWeapon* GetSubWeapon() const;

	/** 장착 X, 획득하는 개념 (땅에 떨어진 무기 줍기, 상점에서 무기 구매) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AcquireWeapon(ABaseWeapon* weapon);
	
	/**해당 슬롯의 인터랙터를 손에 들고자 할 때*/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(EInteractorType InteractorType);

	UFUNCTION(Server, Reliable, Category = "Weapon")
	void Server_SwitchWeapon(EInteractorType InteractorType);
	
	void Reload();
	void Interact();
	void DropCurrentInteractor();

	UFUNCTION(BlueprintCallable)
	float GetEffectSpeedMulitiplier() const { return EffectSpeedMultiplier; }

	UFUNCTION(BlueprintCallable)
	void SetEffectSpeedMultiplier(const float newEffectSpeed) { EffectSpeedMultiplier = newEffectSpeed; }

	UFUNCTION(BlueprintCallable)
	float GetEquipSpeedMuliplier() const { return EquipSpeedMultiplier; }

	UFUNCTION(BlueprintCallable)
	void SetEquipSpeedMultiplier(const float newEquipSpeed) { EquipSpeedMultiplier = newEquipSpeed; }

	UFUNCTION(BlueprintCallable)
	void SetShopUI();


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UValorantGameInstance* m_GameInstance;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// AAgentPlayerState* PS = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AAgentPlayerController* PC = nullptr;

	TWeakObjectPtr<UAgentAbilitySystemComponent> ASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 m_AgentID = 0;

	FAgentData* m_AgentData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAgentAnimInstance* ABP_1P = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAgentAnimInstance* ABP_3P = nullptr;

	UPROPERTY()
	ABaseInteractor* FindInteractActor = nullptr;

	UPROPERTY(Replicated)
	bool bIsRun = true;
	UPROPERTY()
	bool bIsDead = false;

	float BaseCapsuleHalfHeight = 0.0f;

	float BaseRunSpeed = 675.0f;
	float BaseWalkSpeed = 330.0f;

	float EquipSpeedMultiplier = 1.0f;
	float EffectSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FindItemRange = 250.0f;

	UPROPERTY(Replicated)
	ABaseWeapon* MainWeapon = nullptr;
	UPROPERTY(Replicated)
	ABaseWeapon* SubWeapon = nullptr;

	UPROPERTY(Replicated)
	ABaseInteractor* CurrentInteractor = nullptr;

	UPROPERTY(Replicated)
	EInteractorType CurrentInteractorState = EInteractorType::None;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void HandleCrouchProgress(float Value);
	UFUNCTION()
	void HandleDieCamera(FVector newPos);
	UFUNCTION()
	void HandleDieCameraPitch(float newPitch);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void InitAgentAbility();

	UFUNCTION(BlueprintCallable)
	void EquipInteractor(ABaseInteractor* interactor);

	UFUNCTION()
	void OnFindInteraction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnInteractionCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void Die();

	UFUNCTION()
	void OnDieCameraFinished();

	UFUNCTION(NetMulticast, Reliable)
	void Net_Die();

	UFUNCTION()
	void UpdateHealth(float newHealth);
	UFUNCTION()
	void UpdateMaxHealth(float newMaxHealth);
	UFUNCTION()
	void UpdateArmor(float newArmor);
	UFUNCTION()
	void UpdateEffectSpeed(float newEffectSpeed);

	// 무기 카테고리에 따른 이동 속도 멀티플라이어 업데이트
	void UpdateEquipSpeedMultiplier();
};
