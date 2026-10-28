#include "Core/EditorWorldDrawListBuilder.h"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace phoenix::tools::map_editor
{
    std::optional<std::uint32_t> EditorWorldDrawListBuilder::ResolveTileSpriteId(
        const world::LegacyMap& map,
        const std::uint32_t storedSpriteId,
        const world::LegacyTileSpriteIndex* tileLogicalIndex,
        const std::uint32_t physicalTileSpriteCount) noexcept
    {
        if (storedSpriteId == 0xFFFFFFFFU || storedSpriteId == 0xFFFFU) return std::nullopt;
        if (map.parserVariant == world::LegacyMapParserVariant::Map2024LayoutCompatible && tileLogicalIndex != nullptr)
            return tileLogicalIndex->Resolve(storedSpriteId, physicalTileSpriteCount);
        return storedSpriteId < physicalTileSpriteCount
            ? std::optional<std::uint32_t>{ storedSpriteId }
            : std::nullopt;
    }

    world::WorldDrawList EditorWorldDrawListBuilder::BuildFullMap(
        const world::LegacyMap& map,
        const world::LegacyTileSpriteIndex* tileLogicalIndex,
        const std::uint32_t physicalTileSpriteCount) const
    {
        world::WorldDrawList list;
        list.camera.viewport = ui::UiRect{ 0, 0,
            static_cast<std::int32_t>(map.width) * 48,
            static_cast<std::int32_t>(map.height) * 24 };
        list.camera.origin = { 0, 0 };
        list.camera.centerX = map.width == 0U ? 0U : static_cast<std::uint16_t>((map.width - 1U) / 2U);
        list.camera.centerY = map.height == 0U ? 0U : static_cast<std::uint16_t>((map.height - 1U) / 2U);

        list.tiles.reserve(map.sectors.size());
        for (std::uint16_t y = 0; y < map.height; ++y)
        {
            for (std::uint16_t x = 0; x < map.width; ++x)
            {
                const auto* sector = map.Sector(x, y);
                if (sector == nullptr) continue;
                const auto spriteId = ResolveTileSpriteId(
                    map, sector->spriteId, tileLogicalIndex, physicalTileSpriteCount);
                if (!spriteId) continue;
                list.tiles.push_back(world::WorldTileDrawItem{
                    x, y, *spriteId, sector->light,
                    render::ScreenPoint{ static_cast<std::int32_t>(x) * 48, static_cast<std::int32_t>(y) * 24 } });
            }
        }

        list.staticObjects.reserve(map.imageObjects.size());
        for (const auto& object : map.imageObjects)
        {
            if (object.spriteId == 0xFFFFU) continue;
            const auto anchor = render::ScreenPoint{ object.pixelX, object.pixelY };
            list.staticObjects.push_back(world::WorldStaticObjectDrawItem{
                &object, anchor, anchor.y + static_cast<std::int32_t>(object.viewpoint) });
        }

        list.layers.reserve(list.staticObjects.size());
        std::uint64_t insertionOrder = 0U;
        for (std::size_t index = 0; index < list.staticObjects.size(); ++index)
        {
            const auto* object = list.staticObjects[index].object;
            if (object == nullptr) continue;
            list.layers.push_back(world::WorldLayerDrawItem{
                world::WorldLayerDrawKind::StaticObject,
                index,
                list.staticObjects[index].order,
                object->viewpoint == 0U ? world::WorldLayerPass::PreBackground : world::WorldLayerPass::Row,
                static_cast<std::int32_t>(object->viewpoint),
                1U,
                static_cast<std::int32_t>(object->objectId),
                object->objectId,
                insertionOrder++ });
        }
        std::stable_sort(list.layers.begin(), list.layers.end(), [](const auto& left, const auto& right) {
            return std::tie(left.pass, left.legacyRow, left.legacyPriority, left.legacyPrimary,
                       left.legacyObjectId, left.insertionOrder) <
                std::tie(right.pass, right.legacyRow, right.legacyPriority, right.legacyPrimary,
                       right.legacyObjectId, right.insertionOrder);
        });
        return list;
    }

    world::LegacyVisibleSectorBounds EditorWorldDrawListBuilder::ResolveVisibleSectorBounds(
        const std::uint16_t mapWidth,
        const std::uint16_t mapHeight,
        const std::uint16_t centerX,
        const std::uint16_t centerY,
        const ui::UiRect viewport) noexcept
    {
        world::LegacyVisibleSectorBounds bounds;
        if (mapWidth == 0U || mapHeight == 0U) return bounds;
        constexpr std::int32_t margin = 16;
        const auto extentX = std::max(1, static_cast<std::int32_t>(std::ceil(
            static_cast<double>(std::max(0, viewport.width)) * 0.5 / 48.0))) + margin;
        const auto extentY = std::max(1, static_cast<std::int32_t>(std::ceil(
            static_cast<double>(std::max(0, viewport.height)) * 0.5 / 24.0))) + margin;
        bounds.minX = std::max(0, static_cast<std::int32_t>(centerX) - extentX);
        bounds.maxX = std::min(static_cast<std::int32_t>(mapWidth) - 1,
            static_cast<std::int32_t>(centerX) + extentX);
        bounds.minY = std::max(0, static_cast<std::int32_t>(centerY) - extentY);
        bounds.maxY = std::min(static_cast<std::int32_t>(mapHeight) - 1,
            static_cast<std::int32_t>(centerY) + extentY);
        return bounds;
    }

    bool EditorWorldDrawListBuilder::Intersects(const ui::UiRect& left, const ui::UiRect& right) noexcept
    {
        return left.x < right.x + right.width && left.x + left.width > right.x &&
            left.y < right.y + right.height && left.y + left.height > right.y;
    }
}
