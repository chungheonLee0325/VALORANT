#include "HUDAbilityBarModel.h"

#include "GameManager/ValorantGameInstance.h"
#include "Player/AgentPlayerState.h"

bool UHUDAbilityBarModel::Bind(AAgentPlayerState* PlayerState, UValorantGameInstance* GameInstance)
{
    const int32 agentId = PlayerState ? PlayerState->GetAgentID() : INDEX_NONE;
    if (BoundPlayerState.Get() == PlayerState && BoundAgentId == agentId && !BoundPlayerState.IsStale())
    {
        return false;
    }

    Unbind();
    BoundPlayerState = PlayerState;
    BoundAgentId = GameInstance ? agentId : INDEX_NONE;
    if (!PlayerState || !GameInstance || agentId <= 0)
    {
        return true;
    }

    if (const auto* agent = GameInstance->GetAgentData(agentId))
    {
        const int32 ids[] = {agent->AbilityID_C, agent->AbilityID_Q, agent->AbilityID_E, agent->AbilityID_X};
        for (int32 index = 0; index < UE_ARRAY_COUNT(SlotTypes); ++index)
        {
            const int32 id = ids[index];
            if (const auto* ability = id > 0 ? GameInstance->GetAbilityData(id) : nullptr)
            {
                auto& slot = Slots.Add(SlotTypes[index]);
                slot.AbilityID = id;
                slot.AbilityName = ability->AbilityName;
                slot.AbilityDescription = ability->AbilityDescription;
                slot.AbilityIcon = ability->AbilityIcon;
                slot.CurrentStack = PlayerState->GetAbilityStack(id);
                slot.MaxStack = PlayerState->GetMaxAbilityStack(id);
                slot.ChargeCost = ability->ChargeCost;
            }
        }
        PlayerState->OnAbilityStackChanged.AddUniqueDynamic(this, &UHUDAbilityBarModel::OnStackChanged);
    }
    return true;
}

void UHUDAbilityBarModel::Unbind()
{
    if (auto* state = BoundPlayerState.Get())
    {
        state->OnAbilityStackChanged.RemoveDynamic(this, &UHUDAbilityBarModel::OnStackChanged);
    }
    BoundPlayerState.Reset();
    BoundAgentId = INDEX_NONE;
    Slots.Reset();
}

FHUDAbilityInfo UHUDAbilityBarModel::GetSlot(EAbilitySlotType Slot) const
{
    return Slots.FindRef(Slot);
}

FHUDAbilityInfo UHUDAbilityBarModel::GetAbility(int32 AbilityID) const
{
    for (const auto& item : Slots)
    {
        if (item.Value.AbilityID == AbilityID)
        {
            return item.Value;
        }
    }
    return {};
}

int32 UHUDAbilityBarModel::GetCurrentStack(int32 AbilityID) const
{
    return BoundPlayerState.IsValid() ? BoundPlayerState->GetAbilityStack(AbilityID) : 0;
}

int32 UHUDAbilityBarModel::GetMaxStack(int32 AbilityID) const
{
    const auto ability = GetAbility(AbilityID);
    if (ability.AbilityID != 0)
    {
        return ability.MaxStack;
    }
    return BoundPlayerState.IsValid() ? BoundPlayerState->GetMaxAbilityStack(AbilityID) : 0;
}

void UHUDAbilityBarModel::RefreshSlot(EAbilitySlotType Slot, int32 AbilityID)
{
    const auto* item = Slots.Find(Slot);
    if (item && item->AbilityID == AbilityID)
    {
        OnStackChanged(AbilityID, GetCurrentStack(AbilityID));
    }
}

void UHUDAbilityBarModel::OnStackChanged(int32 AbilityID, int32 NewStack)
{
    for (auto& item : Slots)
    {
        if (item.Value.AbilityID == AbilityID && item.Value.CurrentStack != NewStack)
        {
            item.Value.CurrentStack = NewStack;
            OnSlotChanged.Broadcast(item.Key);
        }
    }
}
