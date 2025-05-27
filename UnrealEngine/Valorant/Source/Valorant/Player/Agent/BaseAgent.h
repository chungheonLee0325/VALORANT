#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "Player/Component/FlashComponent.h"
#include "Player/Component/FlashPostProcessComponent.h"
#include "BaseAgent.generated.h"

class UFlashWidget;
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
class UMiniMapWidget;
class AAgentPlayerController;

//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//             CYT             ♣
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//시야 감지 상태 (적이보임, 적이보이지않음 , 마지막보인적의위치)
UENUM(BlueprintType)
enum class EVisibilityState  : uint8
{
	Visible,
	Hidden,
	QuestionMark,
};

UENUM(BlueprintType)
enum class EAgentDamagedPart : uint8
{
	None,
	Head,
	Body,
	Legs
};

UENUM(BlueprintType)
enum class EAgentDamagedDirection : uint8
{
	Front,
	Back,
	Left,
	Right
};

//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//             CYT             ♣
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
// 가시성 정보를 저장할 구조체
USTRUCT()
struct FAgentVisibilityInfo
{
	GENERATED_BODY()

	UPROPERTY()
	ABaseAgent* Observer = nullptr;

	UPROPERTY()
	EVisibilityState VisibilityState = EVisibilityState::Hidden;

	UPROPERTY()
	float QuestionMarkTimer = 0.0f;
	
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAgentDamaged, const EAgentDamagedPart, DamagedPart, const EAgentDamagedDirection, DamagedDirection, const bool, bDie, const bool, bLarge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAgentEquip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAgentFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAgentReload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpikeActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpikeCancel);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpikeDeactive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpikeDefuseFinish);


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
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USkeletalMeshComponent* DefusalMesh;
	
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
	float DieCameraTimeRange = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Die")
	UTimelineComponent* TL_DieCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Die")
	UCurveFloat* DieCameraPitchCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Die")
	UCurveVector* DieCameraCurve;

	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

	// 캐릭터별 미니맵 아이콘을 설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	// 캐릭터의 미니맵 아이콘을 설정하는 함수 선언
	void SetMinimapIcon(UTexture2D* NewIcon);

    // 다른 에이전트에게 보여질 상태 확인 함수
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	// 특정 관찰자에게 현재 에이전트가 어떻게 보이는지 상태를 반환
	EVisibilityState  GetVisibilityStateForAgent(ABaseAgent* Observer);

	// 서버에서 시야 상태 업데이트 함수
	UFUNCTION(Server, Reliable, WithValidation)
	// 서버에서 실행되어 시야 상태를 업데이트
	void Server_UpdateVisibilityState(ABaseAgent* Observer, EVisibilityState NewState);

	// 모든 클라이언트에 시야 상태 전파 함수
	UFUNCTION(NetMulticast, Reliable) 
	// 모든 클라이언트에 시야 상태 변경을 알림
	void Multicast_OnVisibilityStateChanged(ABaseAgent* Observer, EVisibilityState NewState);

	// 미니맵 아이콘 가져오기 함수
	UFUNCTION(BlueprintPure, Category = "Minimap")
	// 현재 설정된 미니맵 아이콘을 반환
	UTexture2D* GetMinimapIcon() const { return MinimapIcon; }

	// 물음표 아이콘 가져오기 함수
	UFUNCTION(BlueprintPure, Category = "Minimap")
	// 물음표 아이콘을 반환
	UTexture2D* GetQuestionMarkIcon() const { return QuestionMarkIcon; }

	// 라인 트레이스로 시야 검사 수행하는 함수 (시야 검사를 수행하여 다른 에이전트의 가시성을 판단하는 함수)
	void PerformVisibilityChecks();

	// 상태 업데이트 헬퍼 함수 
	void UpdateVisibilityState(ABaseAgent* Observer, EVisibilityState NewState);
    
	// 가시성 정보 찾기 헬퍼 함수 
	bool FindVisibilityInfo(ABaseAgent* Observer, FAgentVisibilityInfo& OutInfo, int32& OutIndex); 

	// TMap 대신 OnRep 사용하는 TArray로 변경 
	UPROPERTY(ReplicatedUsing = OnRep_VisibilityStateArray) 
	TArray<FAgentVisibilityInfo> VisibilityStateArray; 

	// OnRep 함수 선언 
	UFUNCTION() 
	void OnRep_VisibilityStateArray(); 

	// 마지막 시야 확인 시간 (최적화용)
	UPROPERTY()
	// 마지막으로 시야 체크를 수행한 시간을 저장
	float LastVisibilityCheckTime;

	// 시야 체크 주기 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	// 시야 체크를 얼마나 자주 수행할지 결정하는 간격 (초 단위)
	float VisibilityCheckInterval;

	// 물음표 표시 지속 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap") 
	// 물음표 상태가 유지되는 시간 (초 단위)
	float QuestionMarkDuration;

	// 캐릭터의 미니맵 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap") 
	// 미니맵에 표시될 캐릭터의 아이콘 텍스처
	UTexture2D* MinimapIcon;

	// 물음표 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	// 시야에서 사라진 적을 표시할 물음표 아이콘 텍스처
	UTexture2D* QuestionMarkIcon;
	
	
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

	UAgentAnimInstance* GetABP_1P() const { return ABP_1P; }
	UAgentAnimInstance* GetABP_3P() const { return ABP_3P; }
	USkeletalMeshComponent* GetMesh1P() const { return FirstPersonMesh; }
	
	bool IsDead() const { return bIsDead; }
	bool CanMove() const { return bCanMove; }
	void SetCanMove(bool canMove) { bCanMove = canMove; }
	
	int GetPoseIdx() const { return PoseIdx; }

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerApplyGE(TSubclassOf<UGameplayEffect> geClass);
	UFUNCTION(Server, Reliable)
	void ServerApplyHitScanGE(TSubclassOf<UGameplayEffect> GEClass, const int Damage,
	                          ABaseAgent* DamageInstigator = nullptr, const EAgentDamagedPart DamagedPart = EAgentDamagedPart::Body, const EAgentDamagedDirection DamagedDirection = EAgentDamagedDirection::Front);

	UFUNCTION(BlueprintCallable)
	void SetIsRun(const bool _bIsRun);
	UFUNCTION(Server, Reliable)
	void Server_SetIsRun(const bool _bIsRun);

	UFUNCTION(Category= "Input")
	void StartFire();
	UFUNCTION(Category= "Input")
	void EndFire();

	UFUNCTION(Category= "Input")
	void Reload();
	UFUNCTION(Category= "Input")
	void Interact();

	UFUNCTION(Server, Reliable, Category= "Input")
	void ServerRPC_DropCurrentInteractor();

	UFUNCTION()
	ABaseInteractor* GetFindInteractorActor() const { return FindInteractActor; }
	void ResetFindInteractorActor() { FindInteractActor = nullptr; }

	UFUNCTION(BlueprintCallable)
	EInteractorType GetInteractorState() const { return CurrentEquipmentState; }

	UFUNCTION(BlueprintCallable)
	ABaseInteractor* GetCurrentInterator() const { return CurrentInteractor; }

	UFUNCTION(Server, Reliable)
	void ServerRPC_SetCurrentInteractor(ABaseInteractor* interactor);

	ABaseWeapon* GetMainWeapon() const;
	ABaseWeapon* GetSubWeapon() const;
	void ResetMainWeapon() { MainWeapon = nullptr; }
	void ResetSubWeapon() { SubWeapon = nullptr; }
	
	ABaseWeapon* GetMeleeWeapon() const;
	void SetMeleeWeapon(ABaseWeapon* knife) { MeleeKnife = knife; }
	
	void ResetOwnSpike();

	/** 장착 X, 획득하는 개념 (땅에 떨어진 무기 줍기, 상점에서 무기 구매) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AcquireInteractor(ABaseInteractor* Interactor);

	/** 해당 슬롯의 인터랙터를 손에 들고자 할 때 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchEquipment(EInteractorType EquipmentType);

	void ActivateSpike();
	void CancelSpike(ASpike* CancelObject);
	UFUNCTION(Server, Reliable)
	void ServerRPC_CancelSpike(ASpike* CancelObject);

	UFUNCTION(Server, Reliable, Category = "Weapon")
	void ServerRPC_Interact(ABaseInteractor* Interactor);
	UFUNCTION(Server, Reliable, Category = "Weapon")
	void Server_AcquireInteractor(ABaseInteractor* Interactor);
	UFUNCTION(Server, Reliable, Category = "Weapon")
	void ServerRPC_SwitchEquipment(EInteractorType InteractorType);

	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetEffectSpeedMulitiplier() const { return EffectSpeedMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "GAS")
	void SetEffectSpeedMultiplier(const float newEffectSpeed) { EffectSpeedMultiplier = newEffectSpeed; }

	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetEquipSpeedMuliplier() const { return EquipSpeedMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "GAS")
	void SetEquipSpeedMultiplier(const float newEquipSpeed) { EquipSpeedMultiplier = newEquipSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetShopUI();

	// 크레딧 보상 함수
	UFUNCTION(BlueprintCallable, Category = "Agent|Credits")
	void AddCredits(int32 Amount);

	// 킬 보상 처리
	UFUNCTION(BlueprintCallable, Category = "Agent|Credits")
	void RewardKill();

	// 스파이크 설치 보상
	UFUNCTION(BlueprintCallable, Category = "Agent|Credits")
	void RewardSpikeInstall();

	UFUNCTION(BlueprintCallable)
	bool GetIsInPlantZone() const { return IsInPlantZone; }
	UFUNCTION(BlueprintCallable)
	void SetIsInPlantZone(bool IsInZone) { IsInPlantZone = IsInZone; }

	// 현재 팀이 블루팀인지 반환
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool IsBlueTeam() const;
	// 현재 팀이 공격팀인지 반환
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool IsAttacker() const;

	EInteractorType GetPrevEquipmentType() const;

	// 1인칭 애니메이션 몽타주 재생
	UFUNCTION(Unreliable, Client, BlueprintCallable, Category = "Animation")
	void Client_PlayFirstPersonMontage(UAnimMontage* MontageToPlay, float PlayRate = 1.0f, FName StartSectionName = NAME_None);

	// 3인칭 애니메이션 몽타주 재생
	UFUNCTION(Unreliable, NetMulticast, BlueprintCallable, Category = "Animation")
	void NetMulti_PlayThirdPersonMontage(UAnimMontage* MontageToPlay, float PlayRate = 1.0f, FName StartSectionName = NAME_None);

	// 1인칭 애니메이션 몽타주 정지
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void StopFirstPersonMontage(float BlendOutTime = 0.25f);

	// 3인칭 애니메이션 몽타주 정지
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void StopThirdPersonMontage(float BlendOutTime = 0.25f);

	// 현재 체력 반환
	UFUNCTION(BlueprintCallable, Category = "Agent|Status")
	float GetCurrentHealth() const;

	// 최대 체력 반환
	UFUNCTION(BlueprintCallable, Category = "Agent|Status")
	float GetMaxHealth() const;

	// 체력이 가득 찼는지 확인
	UFUNCTION(BlueprintCallable, Category = "Agent|Status")
	bool IsFullHealth() const;

	// 섬광 관련 함수들
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void OnFlashIntensityChanged(float NewIntensity);
    
	UFUNCTION(BlueprintCallable, Category = "Flash")
	void CreateFlashWidget();
	
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

	UPROPERTY(Replicated)
	ABaseInteractor* FindInteractActor = nullptr;

	UPROPERTY(Replicated)
	bool bIsRun = true;
	UPROPERTY(Replicated)
	bool bIsDead = false;
	
	bool bCanMove = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseSpringArmHeight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrouchSpringArmHeight = -0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseCapsuleHalfHeight = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrouchCapsuleHalfHeight = 0.0f;

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
	ABaseWeapon* MeleeKnife = nullptr;

	UPROPERTY(Replicated)
	ASpike* Spike = nullptr;

	UPROPERTY(VisibleAnywhere, Replicated)
	ABaseInteractor* CurrentInteractor = nullptr;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_CurrentInteractorState)
	EInteractorType CurrentEquipmentState = EInteractorType::None;

	UPROPERTY(Replicated)
	EInteractorType PrevEquipmentState = EInteractorType::None;
	
	UFUNCTION()
	void OnRep_CurrentInteractorState();

	UPROPERTY(Replicated)
	int PoseIdx = 0;
	int PoseIdxOffset = 0;

	EAgentDamagedPart LastDamagedPart;
	EAgentDamagedDirection LastDamagedDirection;;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Category= "Timeline Handle")
	void HandleCrouchProgress(float Value);
	UFUNCTION(Category= "Timeline Handle")
	void HandleDieCamera(FVector newPos);
	UFUNCTION(Category= "Timeline Handle")
	void HandleDieCameraPitch(float newPitch);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnKill();
	
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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_OnDamaged(const EAgentDamagedPart DamagedPart, const EAgentDamagedDirection DamagedDirection, const bool bDie, const bool bLarge = false);

	// 무기 카테고리에 따른 이동 속도 멀티플라이어 업데이트
	void UpdateEquipSpeedMultiplier();

private:
	// ToDo : 수정
	UPROPERTY(Replicated)
	bool IsInPlantZone = false;

public:
	FOnAgentDamaged OnAgentDamaged;
	FOnAgentEquip OnAgentEquip;
	FOnAgentEquip OnAgentFire;
	FOnAgentEquip OnAgentReload;
	
	FOnSpikeActive OnSpikeActive;
	FOnSpikeCancel OnSpikeCancel;
	
	FOnSpikeDeactive OnSpikeDeactive;
	FOnSpikeDefuseFinish OnSpikeDefuseFinish;

	FTimerHandle DeadTimerHandle;
	
	void OnEquip();
	void OnFire();
	void OnReload();
	
	void OnSpikeStartPlant();
	void OnSpikeFinishPlant();
	
	void OnSpikeStartDefuse();
	void OnSpikeFinishDefuse();
	// void OnSpikeCancelDefuse();
	
	void OnSpikeCancelInteract();
	
	bool bInteractionCapsuleInit = false;
	virtual void OnRep_Controller() override;

	bool IsInFrustum(const AActor* Actor) const;

	UPROPERTY(Replicated)
	FRotator ReplicatedControlRotation = FRotator::ZeroRotator;

private:
	// 섬광 컴포넌트들
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flash", meta = (AllowPrivateAccess = "true"))
	UFlashComponent* FlashComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flash", meta = (AllowPrivateAccess = "true"))
	UFlashPostProcessComponent* PostProcessComponent;

	// UI 위젯
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFlashWidget> FlashWidgetClass;

	UPROPERTY()
	UFlashWidget* FlashWidget;
};
