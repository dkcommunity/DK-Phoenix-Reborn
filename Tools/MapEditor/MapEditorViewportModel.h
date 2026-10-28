#pragma once

#include "World/LegacyMapReader.h"

#include <cstdint>

namespace phoenix::tools::map_editor
{
    struct MapCanvasBounds
    {
        std::int64_t left{ 0 };
        std::int64_t top{ 0 };
        std::int64_t right{ 0 };
        std::int64_t bottom{ 0 };

        [[nodiscard]] std::int64_t Width() const noexcept { return right - left; }
        [[nodiscard]] std::int64_t Height() const noexcept { return bottom - top; }
    };

    struct ScaledSpriteRect
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::int32_t width{ 0 };
        std::int32_t height{ 0 };
    };

    [[nodiscard]] MapCanvasBounds CalculateMapCanvasBounds(const world::LegacyMap& map) noexcept;
    [[nodiscard]] float CalculateFitZoom(
        const MapCanvasBounds& bounds, std::int32_t viewportWidth, std::int32_t viewportHeight,
        std::int32_t padding = 24) noexcept;
    [[nodiscard]] std::int32_t CalculateVisibleTileRadius(
        std::int32_t viewportWidth, std::int32_t viewportHeight, float zoom) noexcept;
    [[nodiscard]] ScaledSpriteRect CalculateScaledSpriteRect(
        std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height,
        float centerX, float centerY, float zoom) noexcept;
}
