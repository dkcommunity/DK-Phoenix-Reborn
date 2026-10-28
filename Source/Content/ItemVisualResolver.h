#pragma once

#include "Core/StrongId.h"

#include <cstdint>

namespace phoenix::content
{
    enum class ItemVisualContext
    {
        Inventory,
        GroundDrop,
        TileObject,
        DescriptionPanel
    };

    struct ItemVisualRequest
    {
        std::uint32_t itemType{ 0 };
        std::uint32_t itemClass{ 0 };
        ItemVisualContext context{ ItemVisualContext::Inventory };
    };

    struct ItemVisualPlan
    {
        core::SpriteId spriteId;
        std::uint32_t gridWidth{ 1 };
        std::uint32_t gridHeight{ 1 };
        bool stackable{ false };
    };
}
