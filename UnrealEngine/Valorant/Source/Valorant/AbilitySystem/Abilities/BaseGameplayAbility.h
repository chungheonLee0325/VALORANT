// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;

// 스킬 입력 유형 정의
UENUM(BlueprintType)
enum class EAbilityInputType : uint8
{
    Instant,            // 즉시 실행 (예: 제트 대쉬)
    Hold,               // 홀드 (예: 소바 화살 차징)
    Toggle,             // 토글 (예: 오멘 텔레포트 모드)
    Sequence,           // 시퀀스 입력 (예: 레이즈 궁극기)
    MultiPhase,         // 다단계 (예: 제트 스모크)
    Repeatable          // 반복 가능 (예: 피닉스 불꽃 벽)
};

// 스킬 단계 정의
UENUM(BlueprintType)
enum class EAbilityPhase : uint8
{
    None,
    Ready,              // 준비 단계
    Aiming,             // 조준 단계
    Charging,           // 차징 단계
    Executing,          // 실행 단계
    Cooldown            // 쿨다운 단계
};

/**
 * 발로란트 스타일 게임의 기본 어빌리티 클래스
 */
UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UBaseGameplayAbility();

    // 어빌리티 ID 설정
    UFUNCTION(BlueprintCallable)
    void SetAbilityID(int32 AbilityID);

    // 후속 입력 태그 관리
    UFUNCTION(BlueprintCallable)
    void SetFollowUpInputTag(FGameplayTag inputTag) { FollowUpInputTags.Add(inputTag); }
    
    UFUNCTION(BlueprintCallable)
    void ClearFollowUpInputTag() { FollowUpInputTags.Empty(); }
    
    // 어빌리티 스택 관리
    UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
    bool ConsumeAbilityStack(const APlayerController* PlayerController);
    
    UFUNCTION(BlueprintCallable, Category = "Ability|Stack")
    int32 GetAbilityStack(const APlayerController* PlayerController) const;

    // 현재 스킬 단계 가져오기
    UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
    EAbilityPhase GetCurrentPhase() const { return CurrentPhase; }

    // 스킬 단계 설정
    UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
    virtual void SetupPhase(EAbilityPhase NewPhase);

    // 다음 단계로 진행
    UFUNCTION(BlueprintCallable, Category = "Ability|Phases")
    virtual void AdvanceToNextPhase();
        
    UFUNCTION()
    virtual void Active_Left_Click(FGameplayEventData data);
    
    UFUNCTION()
    virtual void Active_Right_Click(FGameplayEventData data);
    
protected:
    // 어빌리티 ID (스택 관리용)
    UPROPERTY(EditAnywhere)
    int32 m_AbilityID = 0;
    
    // 후속 입력 태그 (좌클릭/우클릭 등)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
    TSet<FGameplayTag> FollowUpInputTags;

    // 투사체 클래스
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ABaseProjectile> ProjectileClass;

    // 어빌리티 정보 캐시
    UPROPERTY()
    FGameplayAbilityActorInfo m_ActorInfo;
    
    // 어빌리티 입력 설정
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EAbilityInputType InputType = EAbilityInputType::Instant;
    
    // 현재 어빌리티 단계
    UPROPERTY(BlueprintReadOnly, Category = "Ability State")
    EAbilityPhase CurrentPhase = EAbilityPhase::None;
    
    // 현재 반복 횟수 (반복형 스킬용)
    UPROPERTY(BlueprintReadOnly, Category = "Ability State")
    int32 CurrentRepeatCount = 0;
    
    // 최대 반복 횟수 (반복형 스킬용)
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    int32 MaxRepeatCount = 1;
    
    // 홀드 시간 관련 변수
    float HoldStartTime = 0.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    float MinHoldDuration = 0.2f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    float MaxHoldDuration = 2.0f;

    // 타임아웃 설정
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    float PhaseTimeoutDuration = 0.0f;
    
    // GameplayAbility 오버라이드
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    
    virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
    
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;

    // 에이전트 스킬 정리
    void ClearAgentSkill(const FGameplayAbilityActorInfo* ActorInfo);
    
    // 입력 처리 메서드
    virtual void Active_General();

    // 투사체 생성
    UFUNCTION()
    virtual bool SpawnProjectile(const FGameplayAbilityActorInfo& ActorInfo);
    
    // 타임아웃 처리
    UFUNCTION()
    virtual void OnPhaseTimeout();
    // 종료 처리
    UFUNCTION()
    void OnPhaseComplete();

    // 애니메이션 몽타주
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ReadyMontage;

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayReadyAnimation();

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void OnReadyAnimationCompleted();

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
    
    // 타임아웃 설정
    virtual void SetupPhaseTimeout();
    
    // 입력 컨텍스트 설정
    virtual void SetInputContext(bool bToAbilityContext);
};