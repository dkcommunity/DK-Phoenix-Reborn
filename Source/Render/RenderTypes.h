#pragma once

#include "Core/StrongId.h"

#include <cstdint>

namespace phoenix::render
{
    enum class RenderLayer : std::uint8_t
    {
        Ground = 0,
        Shadow = 1,
        Object = 2,
        Creature = 3,
        Effect = 4,
        Ui = 5
    };

    struct ScreenPoint
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
    };

    struct WorldCell
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::int32_t z{ 0 };
    };

    struct RenderSortKey
    {
        RenderLayer layer{ RenderLayer::Ground };
        std::int32_t isoDepth{ 0 };
        std::int32_t screenY{ 0 };
        std::uint32_t sequence{ 0 };

        friend bool operator<(RenderSortKey left, RenderSortKey right) noexcept
        {
            if (left.layer != right.layer)
            {
                return left.layer < right.layer;
            }

            if (left.isoDepth != right.isoDepth)
            {
                return left.isoDepth < right.isoDepth;
            }

            if (left.screenY != right.screenY)
            {
                return left.screenY < right.screenY;
            }

            return left.sequence < right.sequence;
        }
    };

    struct RenderCommand
    {
        RenderSortKey sortKey;
        core::SpriteId spriteId;
        ScreenPoint position;
        ScreenPoint origin;
    };
}
