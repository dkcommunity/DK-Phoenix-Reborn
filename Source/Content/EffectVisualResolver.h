#pragma once

#include "Core/StrongId.h"
#include "Render/RenderTypes.h"

#include <cstdint>
#include <vector>

namespace phoenix::content
{
    enum class EffectAttachment
    {
        WorldCell,
        Creature,
        Screen,
        Hud
    };

    struct EffectVisualRequest
    {
        std::uint32_t effectId{ 0 };
        EffectAttachment attachment{ EffectAttachment::WorldCell };
        core::ActionId actionId;
        core::DirectionId directionId;
    };

    struct EffectVisualPlan
    {
        std::vector<core::SpriteId> sprites;
        render::RenderLayer layer{ render::RenderLayer::Effect };
        bool followsOwner{ false };
    };
}
