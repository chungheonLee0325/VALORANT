#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;

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
	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	bool HasAbilityState(FGameplayTag StateTag) const;

	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	void SetAbilityState(FGameplayTag StateTag, bool bApply = true);

	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	void RemoveAbilityState(FGameplayTag StateTag);

	// === 후속 입력 관리 ===
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

	// === 어빌리티 상태 관리 (태그 기반) ===
	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	FGameplayTag GetCurrentAbilityState() const { return CurrentAbilityState; }

	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	void TransitionToState(FGameplayTag NewState);

	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	void AdvanceToNextState();

	// === 입력 처리 ===
	UFUNCTION()
	virtual void HandleLeftClick(FGameplayEventData EventData);

	UFUNCTION()
	virtual void HandleRightClick(FGameplayEventData EventData);

	UFUNCTION()
	virtual void HandleFollowUpInput(FGameplayTag InputTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnReadyAnimationCompleted();
protected:
	// === 어빌리티 설정 ===
	UPROPERTY(EditAnywhere, Category = "Ability Config")
	int32 m_AbilityID = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	EAbilityInputType InputType = EAbilityInputType::Instant;

	// === 태그 설정 ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	TArray<FGameplayTag> ValidFollowUpInputs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability Tags")
	FGameplayTagContainer RequiredTags;

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
	FGameplayTag CurrentAbilityState;

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
	float StateTimeoutDuration = 10.0f;

	// === 캐시된 정보 ===
	UPROPERTY()
	FGameplayAbilityActorInfo CachedActorInfo;
	
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

	// 상태별 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|State")
	void OnStateEntered(FGameplayTag StateTag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|State")
	void OnStateExited(FGameplayTag StateTag);

	// 유틸리티 함수들
	UFUNCTION(BlueprintCallable, Category = "Ability|Utility")
	bool SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo);

	UFUNCTION(BlueprintCallable, Category = "Ability|Utility")
	void BroadcastAbilityEvent(FGameplayTag EventTag, const FGameplayEventData& EventData);

	// 타이머 관리
	UFUNCTION()
	void OnStateTimeout();

	UFUNCTION()
	void OnAbilityComplete();

	FTimerHandle StateTimeoutHandle;
	FTimerHandle AbilityCompleteHandle;

	// === 내부 함수들 ===
	virtual void SetupStateTimeout();
	virtual void SetInputContext(bool bToAbilityContext);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayReadyAnimation();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void Play1pAnimation(UAnimMontage* AnimMontage);

	// 상태별 이펙트 재생
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

	// 상태별 액션 실행
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Actions")
	void ExecuteStateAction(float HoldTime = 0.0f);
};