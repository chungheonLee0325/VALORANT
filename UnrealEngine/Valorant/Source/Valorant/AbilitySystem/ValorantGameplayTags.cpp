#include "ValorantGameplayTags.h"
#include "GameplayTagsManager.h"

#define REGISTER_TAG(TagVar, TagNameStr) TagVar = TagManager.RequestGameplayTag(FName(TagNameStr));

FValorantGameplayTags& FValorantGameplayTags::Get()
{
    static FValorantGameplayTags Instance;
    return Instance;
}

void FValorantGameplayTags::InitializeNativeTags()
{
    auto& TagManager = UGameplayTagsManager::Get();

    // 기존 입력 태그들
    REGISTER_TAG(InputTag_Ability_Q, "Input.Skill.Q");
    REGISTER_TAG(InputTag_Ability_E, "Input.Skill.E");
    REGISTER_TAG(InputTag_Ability_C, "Input.Skill.C");
    REGISTER_TAG(InputTag_Ability_X, "Input.Skill.X");
    REGISTER_TAG(InputTag_Default_LeftClick, "Input.Default.LeftClick");
    REGISTER_TAG(InputTag_Default_RightClick, "Input.Default.RightClick");
    REGISTER_TAG(InputTag_Default_Repeat, "Input.Default.Repeat");
    
    // 어빌리티 상태 태그들
    REGISTER_TAG(State_Ability_Executing, "State.Ability.Executing");
    REGISTER_TAG(State_Ability_Ready, "State.Ability.Ready");
    REGISTER_TAG(State_Ability_WaitingFollowUp, "State.Ability.WaitingFollowUp");
    REGISTER_TAG(State_Ability_Charging, "State.Ability.Charging");
    REGISTER_TAG(State_Ability_Aiming, "State.Ability.Aiming");
    
    // 어빌리티 차단 태그들
    REGISTER_TAG(Block_Ability_Input, "Block.Ability.Input");
    REGISTER_TAG(Block_Ability_Activation, "Block.Ability.Activation");
    REGISTER_TAG(Block_Movement, "Block.Movement");
    REGISTER_TAG(Block_WeaponSwitch, "Block.WeaponSwitch");
    
    // 어빌리티 단계 태그들
    REGISTER_TAG(Phase_Ready, "Phase.Ready");
    REGISTER_TAG(Phase_Aiming, "Phase.Aiming");
    REGISTER_TAG(Phase_Charging, "Phase.Charging");
    REGISTER_TAG(Phase_Executing, "Phase.Executing");
    REGISTER_TAG(Phase_Cooldown, "Phase.Cooldown");
    
    // 이벤트 태그들
    REGISTER_TAG(Event_Ability_Started, "Event.Ability.Started");
    REGISTER_TAG(Event_Ability_Ended, "Event.Ability.Ended");
    REGISTER_TAG(Event_Ability_Cancelled, "Event.Ability.Cancelled");
    REGISTER_TAG(Event_Ability_PhaseChanged, "Event.Ability.PhaseChanged");
}

#undef REGISTER_TAG