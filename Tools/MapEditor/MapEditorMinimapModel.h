#pragma once

#include "World/LegacyMapReader.h"

#include <cstdint>
#include <vector>

namespace phoenix::tools::map_editor
{
    struct MinimapTerrainCell
    {
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint32_t spriteId{ 0xFFFFFFFFU };
        std::uint8_t property{ 0 };
        std::uint8_t light{ 0 };
    };

    struct MinimapObjectPoint
    {
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        bool animation{ false };
    };

    struct MinimapViewportRect
    {
        float left{ 0.0F };
        float top{ 0.0F };
        float right{ 0.0F };
        float bottom{ 0.0F };
    };

    struct MinimapRenderList
    {
        std::uint16_t mapWidth{ 0 };
        std::uint16_t mapHeight{ 0 };
        std::vector<MinimapTerrainCell> terrain;
        std::vector<MinimapObjectPoint> objects;
        MinimapViewportRect viewport;
    };

    [[nodiscard]] MinimapRenderList BuildMinimapRenderList(
        const world::LegacyMap& map, float cameraX, float cameraY,
        std::int32_t viewportWidth, std::int32_t viewportHeight, float zoom);
    [[nodiscard]] MinimapViewportRect CalculateMinimapViewportRect(
        std::uint16_t mapWidth, std::uint16_t mapHeight, float cameraX, float cameraY,
        std::int32_t viewportWidth, std::int32_t viewportHeight, float zoom) noexcept;
}
