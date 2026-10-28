#pragma once

#include "World/LegacyTileSpriteIndex.h"
#include "World/WorldDrawListTypes.h"

#include <optional>

namespace phoenix::tools::map_editor
{
    class EditorWorldDrawListBuilder final
    {
    public:
        [[nodiscard]] world::WorldDrawList BuildFullMap(
            const world::LegacyMap& map,
            const world::LegacyTileSpriteIndex* tileLogicalIndex,
            std::uint32_t physicalTileSpriteCount) const;

        [[nodiscard]] static std::optional<std::uint32_t> ResolveTileSpriteId(
            const world::LegacyMap& map,
            std::uint32_t storedSpriteId,
            const world::LegacyTileSpriteIndex* tileLogicalIndex,
            std::uint32_t physicalTileSpriteCount) noexcept;

        [[nodiscard]] static world::LegacyVisibleSectorBounds ResolveVisibleSectorBounds(
            std::uint16_t mapWidth,
            std::uint16_t mapHeight,
            std::uint16_t centerX,
            std::uint16_t centerY,
            ui::UiRect viewport) noexcept;

        [[nodiscard]] static bool Intersects(const ui::UiRect& left, const ui::UiRect& right) noexcept;
    };
}
