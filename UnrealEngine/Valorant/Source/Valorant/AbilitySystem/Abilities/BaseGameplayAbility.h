#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;

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
	RepeatKey,
	LeftOrRight,    // 좌클릭 또는 우클릭
	Any             // 모든 후속 입력
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityPhaseChanged, FGameplayTag, NewPhase);

UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
    UBaseGameplayAbility();

    // === 설정 ===
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    int32 m_AbilityID = 0;
    
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EAbilityActivationType ActivationType = EAbilityActivationType::Instant;
    
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EFollowUpInputType FollowUpInputType = EFollowUpInputType::None;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	float FollowUpTime = 10.0f;
    
    // === 애니메이션 (1인칭/3인칭 페어) ===
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* PrepareMontage_1P = nullptr;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* PrepareMontage_3P = nullptr;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* WaitingMontage_1P = nullptr;  // 대기 중 루프 애니메이션
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* WaitingMontage_3P = nullptr;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteMontage_1P = nullptr;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteMontage_3P = nullptr;
    
    // === 투사체 ===
    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    TSubclassOf<class ABaseProjectile> ProjectileClass;

    // === 델리게이트 ===
    UPROPERTY(BlueprintAssignable)
    FOnAbilityPhaseChanged OnPhaseChanged;

	// === 후속 입력 처리 ===
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void HandleFollowUpInput(FGameplayTag InputTag);
protected:
    // === GameplayAbility 오버라이드 ===
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

    // === 어빌리티 실행 흐름 ===
    virtual void StartAbilityExecution();
    virtual void HandlePreparePhase();
    virtual void HandleWaitingPhase();
    virtual void HandleExecutePhase();

	// == 어빌리티 동작 로직 ===
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void ExecuteAbility();

	// === 후속 입력 처리 ===
	// 상속받아서 처리할경우, return 값 주의! 어빌리티를 실행 페이즈로 전이하려면 return true
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual bool OnLeftClickInput();
    
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual bool OnRightClickInput();
    
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual bool OnRepeatInput();
    
    // === 애니메이션 ===
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayMontages(UAnimMontage* Montage1P, UAnimMontage* Montage3P, bool bStopAllMontages = true);
    
    UFUNCTION()
    void OnMontageCompleted();
    
    UFUNCTION()
    void OnMontageBlendOut();
    
    // === 유틸리티 ===
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ability")
    bool SpawnProjectile(FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator);
    
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void SetAbilityPhase(FGameplayTag NewPhase);
    
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ReduceAbilityStack();
    
    UFUNCTION(BlueprintPure, Category = "Ability")
    int32 GetAbilityStack() const;
    
    // === 멀티플레이 ===
    UFUNCTION()
    void StopAllMontages();

	UPROPERTY()
	ABaseProjectile* SpawnedProjectile = nullptr;

	UPROPERTY()
	FGameplayAbilityActorInfo CachedActorInfo;
private:
    UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
    FGameplayTag CurrentPhase;
    
    FTimerHandle WaitingTimeoutHandle;
    
    UFUNCTION()
    void OnRep_CurrentPhase();
    
    void RegisterFollowUpInputs();
    void UnregisterFollowUpInputs();
    
    UFUNCTION()
    void OnWaitingTimeout();
};
