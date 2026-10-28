#pragma once

#include "Runtime/GameRuntimeModels.h"
#include "World/LegacyTileSpriteIndex.h"
#include "World/WorldDrawListTypes.h"

#include <optional>

namespace phoenix::world
{
    class WorldDrawListBuilder final
    {
    public:
        [[nodiscard]] static std::optional<std::uint32_t> ResolveLegacyTileSpriteId(
            const LegacyMap& map, std::uint32_t storedSpriteId,
            const LegacyTileSpriteIndex* logicalIndex = nullptr,
            std::uint32_t physicalSpriteCount = 0x10000U) noexcept;

        [[nodiscard]] WorldDrawList Build(
            const LegacyMap* map,
            const runtime::GameSessionModel& session,
            ui::UiRect viewport,
            std::int32_t tileRadius,
            float outputScale = 1.0F,
            const LegacyTileSpriteIndex* tileLogicalIndex = nullptr,
            std::uint32_t physicalTileSpriteCount = 0x10000U) const;

        [[nodiscard]] static bool Intersects(const ui::UiRect& a, const ui::UiRect& b) noexcept;

        [[nodiscard]] static LegacyVisibleSectorBounds ResolveLegacyVisibleSectorBounds(
            std::uint16_t mapWidth,
            std::uint16_t mapHeight,
            std::uint16_t centerX,
            std::uint16_t centerY,
            ui::UiRect viewport) noexcept;

    private:
        [[nodiscard]] static bool ContainsPoint(const ui::UiRect& rect, render::ScreenPoint point) noexcept;
        [[nodiscard]] static ui::UiRect Expand(ui::UiRect rect, std::int32_t amount) noexcept;
    };
}
