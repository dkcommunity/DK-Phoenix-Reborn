#pragma once

#include "Content/ContentTypes.h"

#include <map>
#include <vector>

namespace phoenix::content
{
    enum class RaceId : std::uint8_t
    {
        Unknown,
        Vampire,
        Slayer,
        Ousters
    };

    enum class EquipmentSlot : std::uint8_t
    {
        Body,
        Head,
        Weapon,
        Shield,
        Accessory
    };

    struct EquipmentVisual
    {
        EquipmentSlot slot{ EquipmentSlot::Body };
        core::BodyId bodyId;
    };

    struct PlayerVisualRequest
    {
        RaceId race{ RaceId::Unknown };
        core::ActionId actionId;
        core::DirectionId directionId;
        std::map<EquipmentSlot, EquipmentVisual> equipment;
    };

    struct PlayerVisualPlan
    {
        core::BodyId baseBodyId;
        std::vector<AnimationDefinition> layers;
    };
}
