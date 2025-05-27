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
    
    // 어빌리티 상태
    FGameplayTag State_Ability_Preparing;    // 준비 애니메이션 재생 중
    FGameplayTag State_Ability_Waiting;      // 입력 대기 중
    FGameplayTag State_Ability_Executing;    // 실행 중
    
    // 어빌리티 차단 태그들
    FGameplayTag Block_Ability_Input;              // "Block.Ability.Input"
    FGameplayTag Block_Ability_Activation;         // "Block.Ability.Activation"
    FGameplayTag Block_Movement;                   // "Block.Movement"
    FGameplayTag Block_WeaponSwitch;               // "Block.WeaponSwitch"

    // 섬광 관련 태그들
    FGameplayTag Flash_Effect;                     // "Flash.Effect"
    FGameplayTag Flash_Intensity;                  // "Flash.Intensity"
    FGameplayTag Flash_Duration;                   // "Flash.Duration"
    FGameplayTag State_Flash_Blinded;              // "State.Flash.Blinded"
    
    // 이벤트 태그들
    FGameplayTag Event_Ability_Started;            // "Event.Ability.Started"
    FGameplayTag Event_Ability_Ended;              // "Event.Ability.Ended"
    FGameplayTag Event_Ability_Cancelled;          // "Event.Ability.Cancelled"
    FGameplayTag Event_Ability_StateChanged;       // "Event.Ability.StateChanged"
    
    static FValorantGameplayTags& Get();
    void InitializeNativeTags();
};