#include "MapEditorMinimapModel.h"

#include <algorithm>

namespace phoenix::tools::map_editor
{
    MinimapViewportRect CalculateMinimapViewportRect(const std::uint16_t mapWidth,
        const std::uint16_t mapHeight, const float cameraX, const float cameraY,
        const std::int32_t viewportWidth, const std::int32_t viewportHeight, const float zoom) noexcept
    {
        MinimapViewportRect viewport;
        const auto scale = std::max(0.02F, zoom);
        const auto halfWidth = static_cast<float>(std::max(0, viewportWidth)) / (2.0F * 48.0F * scale);
        const auto halfHeight = static_cast<float>(std::max(0, viewportHeight)) / (2.0F * 24.0F * scale);
        viewport.left = std::clamp(cameraX - halfWidth, 0.0F, static_cast<float>(mapWidth));
        viewport.top = std::clamp(cameraY - halfHeight, 0.0F, static_cast<float>(mapHeight));
        viewport.right = std::clamp(cameraX + halfWidth, 0.0F, static_cast<float>(mapWidth));
        viewport.bottom = std::clamp(cameraY + halfHeight, 0.0F, static_cast<float>(mapHeight));
        return viewport;
    }

    MinimapRenderList BuildMinimapRenderList(const world::LegacyMap& map, const float cameraX,
        const float cameraY, const std::int32_t viewportWidth, const std::int32_t viewportHeight,
        const float zoom)
    {
        MinimapRenderList list;
        list.mapWidth = map.width;
        list.mapHeight = map.height;
        list.terrain.reserve(map.sectors.size());
        for (std::uint32_t y = 0; y < map.height; ++y)
        {
            for (std::uint32_t x = 0; x < map.width; ++x)
            {
                const auto* sector = map.Sector(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                if (sector == nullptr) continue;
                list.terrain.push_back(MinimapTerrainCell{ static_cast<std::uint16_t>(x),
                    static_cast<std::uint16_t>(y), sector->spriteId, sector->property, sector->light });
            }
        }

        list.objects.reserve(map.imageObjects.size());
        for (const auto& object : map.imageObjects)
        {
            if (object.positions.empty()) continue;
            const auto [x, y] = object.positions.front();
            if (x >= map.width || y >= map.height) continue;
            list.objects.push_back(MinimapObjectPoint{ x, y,
                object.animation || object.wrapperType == 5U || object.wrapperType == 6U || object.wrapperType == 7U });
        }

        list.viewport = CalculateMinimapViewportRect(
            map.width, map.height, cameraX, cameraY, viewportWidth, viewportHeight, zoom);
        return list;
    }
}
