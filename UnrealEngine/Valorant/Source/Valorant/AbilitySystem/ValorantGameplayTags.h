// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FValorantGameplayTags
{
    // 입력 태그
    FGameplayTag InputTag_Ability_Q;
    FGameplayTag InputTag_Ability_E;
    FGameplayTag InputTag_Ability_C;
    FGameplayTag InputTag_Ability_X;
    FGameplayTag InputTag_Default_LeftClick;
    FGameplayTag InputTag_Default_RightClick;
    FGameplayTag InputTag_Default_Repeat;
    
    // 어빌리티 실행 상태 (기존 bool 변수들을 대체)
    FGameplayTag State_Ability_Ready;              // "State.Ability.Ready" (bIsSkillReady = true)
    FGameplayTag State_Ability_WaitingFollowUp;    // "State.Ability.WaitingFollowUp" (후속 입력 대기)
    FGameplayTag State_Ability_Executing;          // "State.Ability.Executing" (bIsSkillClear = false)
    FGameplayTag State_Ability_Charging;           // "State.Ability.Charging"
    FGameplayTag State_Ability_Aiming;             // "State.Ability.Aiming"
    
    // 어빌리티 차단 태그들
    FGameplayTag Block_Ability_Input;              // "Block.Ability.Input"
    FGameplayTag Block_Ability_Activation;         // "Block.Ability.Activation"
    FGameplayTag Block_Movement;                   // "Block.Movement"
    FGameplayTag Block_WeaponSwitch;               // "Block.WeaponSwitch"
    
    // 어빌리티 단계 태그들
    FGameplayTag Phase_Ready;                      // "Phase.Ready"
    FGameplayTag Phase_Aiming;                     // "Phase.Aiming"
    FGameplayTag Phase_Charging;                   // "Phase.Charging"
    FGameplayTag Phase_Executing;                  // "Phase.Executing"
    FGameplayTag Phase_Cooldown;                   // "Phase.Cooldown"
    
    // 이벤트 태그들
    FGameplayTag Event_Ability_Started;            // "Event.Ability.Started"
    FGameplayTag Event_Ability_Ended;              // "Event.Ability.Ended"
    FGameplayTag Event_Ability_Cancelled;          // "Event.Ability.Cancelled"
    FGameplayTag Event_Ability_PhaseChanged;       // "Event.Ability.PhaseChanged"
    
    static FValorantGameplayTags& Get();
    void InitializeNativeTags();
};