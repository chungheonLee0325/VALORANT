#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;

// 스킬 입력 유형 (에디터 편의성을 위해 enum 유지)
UENUM(BlueprintType)
enum class EAbilityInputType : uint8
{
	Instant,
	Hold,
	Toggle,
	Sequence,
	MultiPhase,
	Repeatable
};

// 스킬 단계 (태그와 병행 사용)
UENUM(BlueprintType)
enum class EAbilityPhase : uint8
{
	None,
	Ready,
	Aiming,
	Charging,
	Executing,
	Cooldown
};

UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBaseGameplayAbility();

	// === 어빌리티 설정 ===
	UFUNCTION(BlueprintCallable)
	void SetAbilityID(int32 AbilityID) { m_AbilityID = AbilityID; };

	// === 태그 기반 상태 관리 ===
	UFUNCTION(BlueprintCallable, Category = "Ability|Tags")
	bool HasAbilityState(FGameplayTag StateTag) const;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tags")
	void AddAbilityState(FGameplayTag StateTag);

	UFUNCTION(BlueprintCallable, Category = "Ability|Tags")
	void RemoveAbilityState(FGameplayTag StateTag);

	// === 후속 입력 관리 (개선된 버전) ===
	UFUNCTION(BlueprintCallable, Category = "Ability|Input")
	void RegisterFollowUpInput(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category = "Ability|Input")
	void RegisterFollowUpInputs(const TArray<FGameplayTag>& InputTags);

	UFUNCTION(BlueprintCallable, Category = "Ability|Input")
	void ClearFollowUpInputs();

	UFUNCTION(BlueprintCallable, Category = "Ability|Input")
	bool IsValidFollowUpInput(FGameplayTag InputTag) const;

	// === 어빌리티 스택 관리 ===
	UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
	bool ConsumeAbilityStack(const APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
	int32 GetAbilityStack(const APlayerController* PlayerController) const;

	// === 어빌리티 단계 관리 (개선된 버전) ===
	UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
	EAbilityPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
	FGameplayTag GetCurrentPhaseTag() const;

	UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
	virtual void SetupPhase(EAbilityPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
	virtual void SetupPhaseByTag(FGameplayTag PhaseTag);

	UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
	virtual void AdvanceToNextPhase();

	// === 입력 처리 (개선된 버전) ===
	UFUNCTION()
	virtual void HandleLeftClick(FGameplayEventData EventData);

	UFUNCTION()
	virtual void HandleRightClick(FGameplayEventData EventData);

	UFUNCTION()
	virtual void HandleFollowUpInput(FGameplayTag InputTag, FGameplayEventData EventData);

	// === 기존 함수들 (하위 호환성) ===
	UFUNCTION(BlueprintCallable)
	void SetFollowUpInputTag(FGameplayTag inputTag) { ValidFollowUpInputs.Add(inputTag); }

	UFUNCTION(BlueprintCallable)
	void ClearFollowUpInputTag() { ValidFollowUpInputs.Empty(); }

	UFUNCTION()
	virtual void Active_Left_Click(FGameplayEventData data) { HandleLeftClick(data); }

	UFUNCTION()
	virtual void Active_Right_Click(FGameplayEventData data) { HandleRightClick(data); }

protected:
	// === 어빌리티 설정 ===
	UPROPERTY(EditAnywhere, Category = "Ability Config")
	int32 m_AbilityID = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	EAbilityInputType InputType = EAbilityInputType::Instant;

	// === 태그 설정 ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	FGameplayTagContainer AbilityStateTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	TArray<FGameplayTag> ValidFollowUpInputs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	FGameplayTagContainer RequiredTags;

	// === 기존 후속 입력 태그 (하위 호환성) ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TSet<FGameplayTag> FollowUpInputTags;

	// === 투사체 및 애니메이션 ===
	UPROPERTY(EditDefaultsOnly, Category = "Ability Assets")
	TSubclassOf<ABaseProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Assets")
	UAnimMontage* Ready1pMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Ability Assets")
	UAnimMontage* Ready3pMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Assets")
	UAnimMontage* Execute1pMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Ability Assets")
	UAnimMontage* Execute3pMontage;

	// === 상태 변수들 ===
	UPROPERTY(BlueprintReadOnly, Category = "Ability State")
	EAbilityPhase CurrentPhase = EAbilityPhase::None;

	UPROPERTY(BlueprintReadOnly, Category = "Ability State")
	int32 CurrentRepeatCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ability State")
	float HoldStartTime = 0.0f;

	// === 설정 변수들 ===
	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	int32 MaxRepeatCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	float MinHoldDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	float MaxHoldDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	float PhaseTimeoutDuration = 10.0f;

	// === 캐시된 정보 ===
	UPROPERTY()
	FGameplayAbilityActorInfo CachedActorInfo;

	// 기존 변수 (하위 호환성)
	UPROPERTY()
	FGameplayAbilityActorInfo m_ActorInfo;

	// === GameplayAbility 오버라이드 ===
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                                const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr,
	                                const FGameplayTagContainer* TargetTags = nullptr,
	                                FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle,
	                          const FGameplayAbilityActorInfo* ActorInfo,
	                          const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
	                           const FGameplayAbilityActorInfo* ActorInfo,
	                           const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle,
	                           const FGameplayAbilityActorInfo* ActorInfo,
	                           const FGameplayAbilityActivationInfo ActivationInfo,
	                           bool bReplicateCancelAbility) override;

	// === 내부 처리 함수들 ===
	virtual void InitializeAbility();
	virtual void CleanupAbility();

	// 어빌리티 타입별 처리
	virtual void HandleInstantAbility();
	virtual void HandleHoldAbility();
	virtual void HandleToggleAbility();
	virtual void HandleSequenceAbility();
	virtual void HandleMultiPhaseAbility();
	virtual void HandleRepeatableAbility();

	// 단계별 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Phases")
	void OnPhaseEntered(FGameplayTag PhaseTag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Phases")
	void OnPhaseExited(FGameplayTag PhaseTag);

	// 유틸리티 함수들
	UFUNCTION(BlueprintCallable, Category = "Ability|Utility")
	bool SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo);

	UFUNCTION(BlueprintCallable, Category = "Ability|Utility")
	void BroadcastAbilityEvent(FGameplayTag EventTag, const FGameplayEventData& EventData);

	// 타이머 관리
	UFUNCTION()
	void OnPhaseTimeout();

	UFUNCTION()
	void OnAbilityComplete();

	UFUNCTION()
	void OnPhaseComplete();

	UFUNCTION()
	void OnReadyAnimationCompleted();

	FTimerHandle PhaseTimeoutHandle;
	FTimerHandle AbilityCompleteHandle;

	// === 기존 함수들 (하위 호환성) ===
	virtual void Active_General();
	void ClearAgentSkill(const FGameplayAbilityActorInfo* ActorInfo);
	virtual void SetupPhaseTimeout();
	virtual void SetInputContext(bool bToAbilityContext);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayReadyAnimation();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void Play1pAnimation(UAnimMontage* AnimMontage);

	// 단계별 이펙트 재생
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Effects")
	void PlayReadyEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Effects")
	void PlayAimingEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Effects")
	void PlayChargingEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Effects")
	void PlayExecuteEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Effects")
	void PlayCooldownEffects();

	// 단계별 액션 실행
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Actions")
	void ExecutePhaseAction(float HoldTime = 0.0f);
};
