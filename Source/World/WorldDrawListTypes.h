#pragma once

#include "Render/RenderTypes.h"
#include "UI/UiTypes.h"
#include "World/LegacyMapReader.h"
#include "World/WorldRenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace phoenix::world
{
    struct LegacyVisibleSectorBounds
    {
        std::int32_t minX{ 0 };
        std::int32_t maxX{ -1 };
        std::int32_t minY{ 0 };
        std::int32_t maxY{ -1 };
    };

    struct WorldCamera
    {
        IsometricProjection projection;
        render::ScreenPoint origin;
        render::ScreenPoint playerIso;
        ui::UiRect viewport;
        std::uint16_t centerX{ 0 };
        std::uint16_t centerY{ 0 };
    };

    struct WorldTileDrawItem
    {
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint32_t spriteId{ 0 };
        std::uint8_t light{ 0 };
        render::ScreenPoint anchor;
    };

    struct WorldStaticObjectDrawItem
    {
        const LegacyMapImageObject* object{ nullptr };
        render::ScreenPoint anchor;
        std::int32_t order{ 0 };
    };

    struct WorldCreatureDrawItem
    {
        std::uint32_t objectId{ 0 };
        bool localPlayer{ false };
        bool initialNpcSeed{ false };
        bool fakeCreature{ false };
        std::size_t npcSeedIndex{ 0 };
        render::ScreenPoint anchor;
        std::int32_t order{ 0 };
        // Dynamic creature rect support (for culling)
        std::uint16_t estimatedWidth{ 288 };
        std::uint16_t estimatedHeight{ 352 };
    };

    struct WorldGroundItemDrawItem
    {
        std::uint32_t objectId{ 0 };
        render::ScreenPoint anchor;
        std::int32_t order{ 0 };
    };

    struct WorldEffectDrawItem
    {
        std::uint64_t runtimeId{ 0 };
        render::ScreenPoint anchor;
        std::int32_t order{ 0 };
    };

    enum class WorldLayerDrawKind
    {
        StaticObject,
        GroundItem,
        Creature,
        Effect
    };

    enum class WorldLayerPass : std::uint8_t
    {
        PreBackground,
        Row
    };

    struct WorldLayerDrawItem
    {
        WorldLayerDrawKind kind{ WorldLayerDrawKind::StaticObject };
        std::size_t index{ 0 };
        std::int32_t order{ 0 };
        WorldLayerPass pass{ WorldLayerPass::Row };
        std::int32_t legacyRow{ 0 };
        std::uint8_t legacyPriority{ 0 };
        std::int32_t legacyPrimary{ 0 };
        std::uint32_t legacyObjectId{ 0 };
        std::uint64_t insertionOrder{ 0 };
    };

    struct WorldDrawList
    {
        WorldCamera camera;
        std::vector<WorldTileDrawItem> tiles;
        std::vector<WorldStaticObjectDrawItem> staticObjects;
        std::vector<WorldGroundItemDrawItem> groundItems;
        std::vector<WorldCreatureDrawItem> creatures;
        std::vector<WorldEffectDrawItem> effects;
        std::vector<WorldLayerDrawItem> layers;
        std::size_t culledTiles{ 0 };
        std::size_t culledObjects{ 0 };
        std::uint64_t layerSortMicroseconds{ 0 };
        std::size_t scannedWorldObjects{ 0 };
        std::size_t scannedStaticObjects{ 0 };
        std::size_t visitedSectors{ 0 };
        std::size_t sectorObjectRecords{ 0 };
        std::size_t deduplicatedStaticObjects{ 0 };
        LegacyVisibleSectorBounds previousBounds{ 0, -1, 0, -1 };
        // Expanded bounds for object culling with extra margin for pixel-positioned objects
        LegacyVisibleSectorBounds expandedObjectBounds{ 0, -1, 0, -1 };
    };
}
