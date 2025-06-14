#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "ResourceManager/ValorantGameType.h"
#include "NiagaraSystem.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;
class UAgentAbilitySystemComponent;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;

UENUM(BlueprintType)
enum class EAbilityActivationType : uint8
{
    Instant,        // 즉시 실행
    WithPrepare     // 준비 -> 대기 -> 실행
};

UENUM(BlueprintType)
enum class EFollowUpInputType : uint8
{
    None,
    LeftClick,
    RightClick,
    LeftOrRight
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStateChanged, FGameplayTag, StateTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrepareAbility, FGameplayTag, SlotTag, EFollowUpInputType, InputType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFollowUpInput);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndAbility, EFollowUpInputType, InputType);

UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UBaseGameplayAbility();

    // 상태 조회 (태그 기반)
    UFUNCTION(BlueprintPure, Category = "Ability|State")
    bool IsInPreparingState() const;
    
    UFUNCTION(BlueprintPure, Category = "Ability|State")
    bool IsInWaitingState() const;
    
    UFUNCTION(BlueprintPure, Category = "Ability|State")
    bool IsInExecutingState() const;

    // 설정
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    int32 m_AbilityID = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EAbilityActivationType ActivationType = EAbilityActivationType::Instant;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EFollowUpInputType FollowUpInputType = EFollowUpInputType::None;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    float FollowUpTime = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    bool bAllowCancelDuringExecution = false;

    // 애니메이션
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* PrepareMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* PrepareMontage_3P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* WaitingMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* WaitingMontage_3P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteMontage_3P = nullptr;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteLeftMouseButtonMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteLeftMouseButtonMontage_3P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteRightMouseButtonMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteRightMouseButtonMontage_3P = nullptr;
    
    // 투사체
    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    // 효과
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* PrepareEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* PrepareSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* WaitEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* WaitSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* ExecuteEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* ExecuteSound = nullptr;

    // 델리게이트
    UPROPERTY(BlueprintAssignable)
    FOnAbilityStateChanged OnStateChanged;

    UPROPERTY()
    FOnPrepareAbility OnWaitAbility;
    
    UPROPERTY()
    FOnEndAbility OnEndAbility;
    
    UPROPERTY()
    FOnFollowUpInput OnFollowUpInput;

    UPROPERTY(BlueprintReadWrite, Category = "Ability", EditAnywhere)
    UNiagaraSystem* ProjectileLaunchEffect;
    
    UPROPERTY(BlueprintReadWrite, Category = "Ability", EditAnywhere)
    USoundBase* ProjectileLaunchSound;

protected:
    // GameplayAbility 오버라이드
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
    
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                           const FGameplayAbilityActorInfo* ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo,
                           bool bReplicateEndAbility, bool bWasCancelled) override;

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                   const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayTagContainer* SourceTags = nullptr,
                                   const FGameplayTagContainer* TargetTags = nullptr,
                                   FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, 
                              const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo,
                              bool bReplicateCancelAbility) override;

    // 어빌리티 플로우 함수들
    void StartPreparePhase();
    void StartWaitingPhase();
    void StartExecutePhase(EFollowUpInputType InputType = EFollowUpInputType::None);
    void CompleteAbility();

    // 어빌리티 동작 - 서브클래스에서 오버라이드
    virtual void PrepareAbility() {}
    virtual void WaitAbility() {}
    virtual void ExecuteAbility() {}

    // 후속 입력 핸들러 - 서브클래스에서 오버라이드
    virtual bool OnLeftClickInput() { return true; }
    virtual bool OnRightClickInput() { return true; }

    // 유틸리티
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ability")
    bool SpawnProjectile(FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Effects")
    void PlayCommonEffects(UNiagaraSystem* NiagaraEffect, USoundBase* SoundEffect, FVector Location = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ReduceAbilityStack();

    UFUNCTION(BlueprintPure, Category = "Ability")
    int32 GetAbilityStack() const;

    // 애니메이션
    void PlayMontages(UAnimMontage* Montage1P, UAnimMontage* Montage3P, bool bStopAllMontages = true);
    void StopAllMontages();

    // 태스크 콜백
    UFUNCTION()
    void OnPrepareMontageCompleted();

    UFUNCTION()
    void OnPrepareMontageBlendOut();

    UFUNCTION()
    void OnExecuteMontageCompleted();

    UFUNCTION()
    void OnExecuteMontageBlendOut();

    UFUNCTION()
    void OnWaitingTimeout();

    UFUNCTION()
    void OnFollowUpEventReceived(FGameplayEventData Payload);

    // 상태 관리 헬퍼
    void SetAbilityState(const FGameplayTag& StateTag);
    void ClearAllAbilityStates();
    FGameplayTag GetCurrentStateTag() const;

    // 캐시된 정보
    UPROPERTY()
    FGameplayAbilityActorInfo CachedActorInfo;

    UPROPERTY()
    UAgentAbilitySystemComponent* CachedASC = nullptr;

    UPROPERTY()
    ABaseProjectile* SpawnedProjectile = nullptr;

private:
    // 이전 장비 상태
    UPROPERTY()
    EInteractorType PreviousEquipmentState = EInteractorType::None;

    // 활성 태스크들
    UPROPERTY()
    UAbilityTask_PlayMontageAndWait* ActiveMontageTask = nullptr;

    UPROPERTY()
    UAbilityTask_WaitGameplayEvent* WaitEventTask = nullptr;

    // 타이머
    FTimerHandle WaitingTimeoutHandle;
    FTimerHandle CleanupDelayHandle;

    // 마지막 입력 타입
    EFollowUpInputType LastInputType = EFollowUpInputType::None;

    // 정리 함수
    void PerformFinalCleanup();
};