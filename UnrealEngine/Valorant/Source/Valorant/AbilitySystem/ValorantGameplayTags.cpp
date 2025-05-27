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

    // 입력 태그들
    REGISTER_TAG(InputTag_Ability_Q, "Input.Skill.Q");
    REGISTER_TAG(InputTag_Ability_E, "Input.Skill.E");
    REGISTER_TAG(InputTag_Ability_C, "Input.Skill.C");
    REGISTER_TAG(InputTag_Ability_X, "Input.Skill.X");
    REGISTER_TAG(InputTag_Default_LeftClick, "Input.Default.LeftClick");
    REGISTER_TAG(InputTag_Default_RightClick, "Input.Default.RightClick");
    REGISTER_TAG(InputTag_Default_Repeat, "Input.Default.Repeat");
    
    // 어빌리티 상태 태그들 (State만 사용)
    REGISTER_TAG(State_Ability_Preparing, "State.Ability.Preparing");
    REGISTER_TAG(State_Ability_Waiting, "State.Ability.Waiting");
    REGISTER_TAG(State_Ability_Executing, "State.Ability.Executing");
    
    // 어빌리티 차단 태그들
    REGISTER_TAG(Block_Ability_Input, "Block.Ability.Input");
    REGISTER_TAG(Block_Ability_Activation, "Block.Ability.Activation");
    REGISTER_TAG(Block_Movement, "Block.Movement");
    REGISTER_TAG(Block_WeaponSwitch, "Block.WeaponSwitch");

    // 섬광 관련 태그들 등록
    REGISTER_TAG(Flash_Effect, "Flash.Effect");
    REGISTER_TAG(Flash_Intensity, "Flash.Intensity");
    REGISTER_TAG(Flash_Duration, "Flash.Duration");
    REGISTER_TAG(State_Flash_Blinded, "State.Flash.Blinded");
    
    // 이벤트 태그들
    REGISTER_TAG(Event_Ability_Started, "Event.Ability.Started");
    REGISTER_TAG(Event_Ability_Ended, "Event.Ability.Ended");
    REGISTER_TAG(Event_Ability_Cancelled, "Event.Ability.Cancelled");
    REGISTER_TAG(Event_Ability_StateChanged, "Event.Ability.StateChanged");
}

#undef REGISTER_TAG