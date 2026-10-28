#include "World/WorldDrawListBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <tuple>
#include <unordered_set>

namespace phoenix::world
{
    std::optional<std::uint32_t> WorldDrawListBuilder::ResolveLegacyTileSpriteId(
        const LegacyMap& map, const std::uint32_t storedSpriteId,
        const LegacyTileSpriteIndex* logicalIndex, const std::uint32_t physicalSpriteCount) noexcept
    {
        if (storedSpriteId == 0xFFFFFFFFU || storedSpriteId == 0xFFFFU) return std::nullopt;
        if (map.parserVariant != LegacyMapParserVariant::Map2024LayoutCompatible)
            return storedSpriteId < physicalSpriteCount ? std::optional<std::uint32_t>{ storedSpriteId } : std::nullopt;
        if (logicalIndex != nullptr)
        {
            if (const auto physical = logicalIndex->Resolve(storedSpriteId, physicalSpriteCount)) return physical;
            return std::nullopt;
        }
        return storedSpriteId < physicalSpriteCount ? std::optional<std::uint32_t>{ storedSpriteId } : std::nullopt;
    }

    LegacyVisibleSectorBounds WorldDrawListBuilder::ResolveLegacyVisibleSectorBounds(
        const std::uint16_t mapWidth,
        const std::uint16_t mapHeight,
        const std::uint16_t centerX,
        const std::uint16_t centerY,
        const ui::UiRect viewport) noexcept
    {
        LegacyVisibleSectorBounds bounds;
        if (mapWidth == 0U || mapHeight == 0U)
        {
            return bounds;
        }

        const IsometricProjection projection;
        // Legacy parity: Sector grid dimensions derived from viewport resolution
        // Legacy MViewDef.h: SECTOR_WIDTH = ceil(SURFACE_WIDTH / TILE_X), SECTOR_HEIGHT = ceil(SURFACE_HEIGHT / TILE_Y)
        // TILE_X = 48, TILE_Y = 24 (IsometricProjection::tileWidth/Height)
        const auto sectorWidth = std::max(1, (viewport.width + projection.tileWidth - 1) / projection.tileWidth);
        const auto sectorHeight = std::max(1, (viewport.height + projection.tileHeight - 1) / projection.tileHeight);

        // Legacy SECTOR_SKIP_PLAYER_LEFT = -SECTOR_WIDTH_HALF
        // Legacy SECTOR_SKIP_PLAYER_UP = -SECTOR_HEIGHT_HALF
        auto firstX = static_cast<std::int32_t>(centerX) - (sectorWidth / 2);
        auto firstY = static_cast<std::int32_t>(centerY) - (sectorHeight / 2);

        // Clamp to map bounds (Legacy MTopView::DetermineImageObject logic)
        if (firstX <= 0) firstX = 0;
        else if (firstX + sectorWidth + 1 >= static_cast<std::int32_t>(mapWidth))
            firstX = std::max(0, static_cast<std::int32_t>(mapWidth) - sectorWidth - 1);
        if (firstY <= 0) firstY = 0;
        else if (firstY + sectorHeight + 1 >= static_cast<std::int32_t>(mapHeight))
            firstY = std::max(0, static_cast<std::int32_t>(mapHeight) - sectorHeight - 1);

        // Legacy TILESURFACE_SECTOR_EDGE = 3
        constexpr int kSectorEdgeMargin = 3;
        bounds.minX = std::max(0, firstX - kSectorEdgeMargin);
        bounds.maxX = std::min(static_cast<std::int32_t>(mapWidth) - 1, firstX + sectorWidth + kSectorEdgeMargin);
        bounds.minY = std::max(0, firstY - kSectorEdgeMargin);
        bounds.maxY = std::min(static_cast<std::int32_t>(mapHeight) - 1, firstY + sectorHeight + kSectorEdgeMargin);
        return bounds;
    }

    bool WorldDrawListBuilder::Intersects(const ui::UiRect& a, const ui::UiRect& b) noexcept
    {
        return a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y;
    }

    bool WorldDrawListBuilder::ContainsPoint(const ui::UiRect& rect, const render::ScreenPoint point) noexcept
    {
        return point.x >= rect.x &&
            point.y >= rect.y &&
            point.x < rect.x + rect.width &&
            point.y < rect.y + rect.height;
    }

    ui::UiRect WorldDrawListBuilder::Expand(ui::UiRect rect, const std::int32_t amount) noexcept
    {
        rect.x -= amount;
        rect.y -= amount;
        rect.width += amount * 2;
        rect.height += amount * 2;
        return rect;
    }

    namespace
    {
        [[nodiscard]] ui::UiRect RectAround(
            const render::ScreenPoint anchor,
            const std::int32_t left,
            const std::int32_t top,
            const std::int32_t width,
            const std::int32_t height) noexcept
        {
            return ui::UiRect{
                anchor.x - left,
                anchor.y - top,
                width,
                height
            };
        }

        render::ScreenPoint ProjectIsometricFloat(
            const float x,
            const float y,
            const render::ScreenPoint origin,
            const IsometricProjection& projection) noexcept
        {
            return render::ScreenPoint{
                origin.x + static_cast<std::int32_t>(std::lround(x * static_cast<float>(projection.tileWidth))),
                origin.y + static_cast<std::int32_t>(std::lround(y * static_cast<float>(projection.tileHeight)))
            };
        }
    }

    WorldDrawList WorldDrawListBuilder::Build(
        const LegacyMap* map,
        const runtime::GameSessionModel& session,
        const ui::UiRect viewport,
        const std::int32_t tileRadius,
        const float outputScale,
        const LegacyTileSpriteIndex* tileLogicalIndex,
        const std::uint32_t physicalTileSpriteCount) const
    {
        WorldDrawList list;
        list.camera.viewport = viewport;

        // Camera setup - use server position for stable culling, visual position for smooth camera
                const auto centerVisualX = session.player.positionKnown ? session.player.motion.visualX : static_cast<float>(session.player.x);
                const auto centerVisualY = session.player.positionKnown ? session.player.motion.visualY : static_cast<float>(session.player.y);
                list.camera.centerX = session.player.x;
                list.camera.centerY = session.player.y;

                list.camera.playerIso = ProjectIsometric(
                    session.player.x,
                    session.player.y,
                    render::ScreenPoint{ 0, 0 },
                    list.camera.projection);

                // Camera origin uses visual position for smooth interpolation
                const auto playerVisualIso = ProjectIsometricFloat(
                    centerVisualX,
                    centerVisualY,
                    render::ScreenPoint{ 0, 0 },
                    list.camera.projection);

                list.camera.origin = render::ScreenPoint{
                    viewport.x + (viewport.width / 2) - playerVisualIso.x - (list.camera.projection.tileWidth / 2),
                    viewport.y + (viewport.height / 2) - playerVisualIso.y - (list.camera.projection.tileHeight / 2)
                };

                const auto worldToScreen = [&list](const std::uint16_t x, const std::uint16_t y) {
                    return ProjectIsometric(x, y, list.camera.origin, list.camera.projection);
                };
                const auto worldToScreenFloat = [&list](const float x, const float y) {
                    return ProjectIsometricFloat(x, y, list.camera.origin, list.camera.projection);
                };

                const auto visibilityScale = std::max(0.02F, outputScale);
                const auto visibilityWidth = static_cast<std::int32_t>(std::ceil(viewport.width / visibilityScale));
                const auto visibilityHeight = static_cast<std::int32_t>(std::ceil(viewport.height / visibilityScale));
                const ui::UiRect visibilityViewport{
                    viewport.x + (viewport.width - visibilityWidth) / 2,
                    viewport.y + (viewport.height - visibilityHeight) / 2,
                    visibilityWidth,
                    visibilityHeight
                };

                // Culling uses SERVER position for stability (matches Legacy)
                // Camera origin uses VISUAL position for smooth rendering
                const auto tileCullRect = Expand(visibilityViewport, 128);  // 128px margin for tile culling (was 96)

                if (map != nullptr && map->width > 0 && map->height > 0)
                {
                    list.scannedStaticObjects = 0;
                    // Use viewport-based sector bounds for tiles (Legacy parity: same grid as objects)
                    // Legacy MTopView::DetermineImageObject uses same sector grid for tiles and objects
                    const auto tileBounds = ResolveLegacyVisibleSectorBounds(
                        map->width, map->height,
                        session.player.x, session.player.y,
                        viewport);
                    
                    list.tiles.reserve(static_cast<std::size_t>(
                        (tileBounds.maxX - tileBounds.minX + 1) * 
                        (tileBounds.maxY - tileBounds.minY + 1)));
                    for (std::int32_t y = tileBounds.minY; y <= tileBounds.maxY; ++y)
                    {
                        for (std::int32_t x = tileBounds.minX; x <= tileBounds.maxX; ++x)
                        {
                            const auto* sector = map->Sector(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                            if (sector == nullptr)
                            {
                                // Empty sector - still count as visited but no sprites
                                continue;
                            }

                            const auto anchor = worldToScreen(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                            // Frustum culling for tiles (Legacy parity: only draw visible sectors)
                            const auto tileRect = RectAround(anchor, 24, 24, 48, 48);
                            if (!Intersects(tileCullRect, tileRect))
                            {
                                ++list.culledTiles;
                                continue;
                            }

                            ++list.visitedSectors;
                            const auto renderSpriteId = ResolveLegacyTileSpriteId(
                                *map, sector->spriteId, tileLogicalIndex, physicalTileSpriteCount);
                            if (!renderSpriteId)
                            {
                                continue;
                            }
                            list.tiles.push_back(WorldTileDrawItem{
                                static_cast<std::uint16_t>(x),
                                static_cast<std::uint16_t>(y),
                                *renderSpriteId,
                                sector->light,
                                anchor
                            });
                        }
                    }

                    list.staticObjects.reserve(std::min<std::size_t>(map->imageObjects.size(), 512));
                    // Use SAME bounds as tiles (Legacy parity: tiles and objects share same sector grid)
                    // Legacy MTopView::DetermineImageObject uses same sector grid for both
                    const auto objectBounds = tileBounds;
                    // 2026-08-31 CULL FIX: +32 sektorluk genis pencere artik SADECE
                    // texture prefetch penceresi (Application::PrefetchSectorDelta).
                    // Admission taramasi legacy parity geregi tileBounds ile ayni
                    // sektor grid'inde yapilir (asagida).
                    constexpr int kObjectSectorMargin = 32;
                    LegacyVisibleSectorBounds expandedObjectBounds = objectBounds;
                    expandedObjectBounds.minX = std::max(0, objectBounds.minX - kObjectSectorMargin);
                    expandedObjectBounds.maxX = std::min(static_cast<std::int32_t>(map->width) - 1, objectBounds.maxX + kObjectSectorMargin);
                    expandedObjectBounds.minY = std::max(0, objectBounds.minY - kObjectSectorMargin);
                    expandedObjectBounds.maxY = std::min(static_cast<std::int32_t>(map->height) - 1, objectBounds.maxY + kObjectSectorMargin);

                    // Prefetch penceresi disari aktarilir (Application gercek "onceki
                    // frame" bounds'unu kendi uyesinde tutar — StreamingPrefetchFix).
                    list.previousBounds = expandedObjectBounds;
                    list.expandedObjectBounds = expandedObjectBounds;

                    // 2026-08-31 CULL FIX (root cause): eski kod admission'i objenin
                    // TEK anchor noktasi etrafinda sabit 128x128 piksel kutuyla
                    // reddediyordu. Buyuk/cok parcali sprite'larda (binalar, yuzlerce
                    // px) anchor viewport+margin disina cikinca govde ekranda oldugu
                    // halde obje komple dusuyordu -> eksik/yarim yuklenmis gorunen
                    // binalar, kenarda kaybolan objeler. Referans client'ta
                    // (MTopView::DetermineImageObject) piksel cull YOK: obje, kapladigi
                    // sektorlerden biri gorunur sektor araligindaysa cizilir (footprint
                    // admission). Ayni model burada uygulanir; gercek boyutlu rect
                    // testi zaten cizim aninda texture boyutuyla yapiliyor
                    // (Application::queueStaticObject -> Intersects(rect, worldRect)).
                    // Eski "hysteresis" blogu ayni frame'in degerlerini okudugu icin
                    // fiilen no-op idi; footprint admission gercek cozumu sagladigindan
                    // tarama araligi legacy grid'e (objectBounds) dondu.
                    std::unordered_set<std::uint32_t> admittedObjectIds;
                    for (auto y = objectBounds.minY; y <= objectBounds.maxY; ++y)
                    {
                        for (auto x = objectBounds.minX; x <= objectBounds.maxX; ++x)
                        {
                            const auto* objectIndices = map->SectorImageObjects(
                                static_cast<std::uint16_t>(x),
                                static_cast<std::uint16_t>(y));
                            if (objectIndices == nullptr)
                            {
                                continue;
                            }
                            list.sectorObjectRecords += objectIndices->size();
                            for (const auto objectIndex : *objectIndices)
                            {
                                ++list.scannedStaticObjects;
                                if (objectIndex >= map->imageObjects.size())
                                {
                                    continue;
                                }
                                const auto& object = map->imageObjects[objectIndex];
                                if (!admittedObjectIds.insert(object.objectId).second)
                                {
                                    ++list.deduplicatedStaticObjects;
                                    continue;
                                }
                                // Don't skip objects with spriteId == 0xFFFFU - they might be invisible/collision objects
                                // that still need to be rendered with fallback or for interaction
                                const auto anchor = render::ScreenPoint{
                                    list.camera.origin.x + object.pixelX,
                                    list.camera.origin.y + object.pixelY
                                };
                                // 2026-08-31 CULL FIX: anchor etrafinda sabit 128x128
                                // piksel reddi kaldirildi (yukaridaki blok yorumuna bak).
                                // Admission = sektor footprint (bu dongune girmis olmak
                                // yeterli); gercek boyutlu ekran-rect testi cizim aninda
                                // texture olculeriyle yapilir. Buyuk sprite'lar artik
                                // anchor konumundan bagimsiz dogru cizilir.
                                list.staticObjects.push_back(WorldStaticObjectDrawItem{
                                    &object,
                                    anchor,
                                    anchor.y + static_cast<std::int32_t>(object.viewpoint)
                                });
                            }
                        }
                    }

                    std::sort(list.staticObjects.begin(), list.staticObjects.end(), [](const auto& left, const auto& right) {
                        return left.order < right.order;
                    });
                }

        const auto creatureCullRect = Expand(visibilityViewport, 256);  // 256 for creatures (reduced from 512)
        list.groundItems.reserve(session.world.groundItems.size());
        for (const auto& [objectId, item] : session.world.groundItems)
        {
            const auto anchor = worldToScreen(item.x, item.y);
            const auto itemRect = RectAround(anchor, 48, 80, 112, 128);
            if (!Intersects(creatureCullRect, itemRect))
            {
                continue;
            }
            list.groundItems.push_back(WorldGroundItemDrawItem{
                objectId,
                anchor,
                anchor.y + 12
            });
        }

        list.creatures.reserve(
            session.world.objects.size() +
            session.world.fakeCreatures.size() +
            session.world.initialNpcSeeds.size() + 1);
        list.scannedWorldObjects = session.world.objects.size();
        for (const auto& [objectId, object] : session.world.objects)
        {
            if (objectId == session.player.objectId)
            {
                continue;
            }
            const auto anchor = worldToScreenFloat(object.motion.visualX, object.motion.visualY);
            // Dynamic creature rect based on estimated sprite size (default 288x352, can be overridden by texture)
            const auto creatureRect = RectAround(anchor, 128, 224, 288, 352);
            if (!Intersects(creatureCullRect, creatureRect))
            {
                continue;
            }
            list.creatures.push_back(WorldCreatureDrawItem{
                objectId,
                false,
                false,
                false,
                0,
                anchor,
                anchor.y + 32,
                288,  // estimatedWidth
                352   // estimatedHeight
            });
        }

        for (const auto& [fakeId, object] : session.world.fakeCreatures)
        {
            const auto anchor = worldToScreenFloat(object.motion.visualX, object.motion.visualY);
            const auto creatureRect = RectAround(anchor, 128, 224, 288, 352);
            if (!Intersects(creatureCullRect, creatureRect))
            {
                continue;
            }
            list.creatures.push_back(WorldCreatureDrawItem{
                fakeId,
                false,
                false,
                true,
                0,
                anchor,
                anchor.y + 32,
                288,  // estimatedWidth
                352   // estimatedHeight
            });
        }

        for (std::size_t i = 0; i < session.world.initialNpcSeeds.size(); ++i)
        {
            const auto& seed = session.world.initialNpcSeeds[i];
            if (seed.hidden)
            {
                continue;
            }
            const auto anchor = worldToScreen(seed.x, seed.y);
            const auto creatureRect = RectAround(anchor, 128, 224, 288, 352);
            if (!Intersects(creatureCullRect, creatureRect))
            {
                continue;
            }
            list.creatures.push_back(WorldCreatureDrawItem{
                0,
                false,
                true,
                false,
                i,
                anchor,
                anchor.y + 32,
                288,  // estimatedWidth
                352   // estimatedHeight
            });
        }

        const auto playerAnchor = worldToScreenFloat(centerVisualX, centerVisualY);
        list.creatures.push_back(WorldCreatureDrawItem{
            session.player.objectId,
            true,
            false,
            false,
            0,
            playerAnchor,
            playerAnchor.y + 32,
            288,  // estimatedWidth
            352   // estimatedHeight
        });
        std::sort(list.creatures.begin(), list.creatures.end(), [](const auto& left, const auto& right) {
            return left.order < right.order;
        });

        list.effects.reserve(session.world.effects.size());
        for (const auto& effect : session.world.effects)
        {
            render::ScreenPoint anchor{};
            if (effect.attachment == runtime::WorldEffectAttachment::Tile)
            {
                anchor = worldToScreen(effect.x, effect.y);
            }
            else if (effect.objectId == session.player.objectId)
            {
                anchor = playerAnchor;
            }
            else if (const auto object = session.world.objects.find(effect.objectId); object != session.world.objects.end())
            {
                anchor = worldToScreenFloat(object->second.motion.visualX, object->second.motion.visualY);
            }
            else
            {
                continue;
            }

            const auto effectRect = RectAround(anchor, 128, 192, 288, 320);
            if (!Intersects(creatureCullRect, effectRect))
            {
                continue;
            }
            list.effects.push_back(WorldEffectDrawItem{
                effect.runtimeId,
                anchor,
                anchor.y + 40
            });
        }

        list.layers.reserve(list.staticObjects.size() + list.groundItems.size() + list.creatures.size() + list.effects.size());
        std::uint64_t insertionOrder = 0;
        for (std::size_t i = 0; i < list.staticObjects.size(); ++i)
        {
            const auto* object = list.staticObjects[i].object;
            if (object == nullptr)
            {
                continue;
            }
            list.layers.push_back(WorldLayerDrawItem{
                WorldLayerDrawKind::StaticObject,
                i,
                list.staticObjects[i].order,
                object->viewpoint == 0U ? WorldLayerPass::PreBackground : WorldLayerPass::Row,
                static_cast<std::int32_t>(object->viewpoint),
                1U,
                static_cast<std::int32_t>(object->objectId),
                object->objectId,
                insertionOrder++ });
        }
        for (std::size_t i = 0; i < list.groundItems.size(); ++i)
        {
            const auto found = session.world.groundItems.find(list.groundItems[i].objectId);
            const auto row = found != session.world.groundItems.end()
                ? static_cast<std::int32_t>(found->second.y)
                : 0;
            list.layers.push_back(WorldLayerDrawItem{
                WorldLayerDrawKind::GroundItem,
                i,
                list.groundItems[i].order,
                WorldLayerPass::Row,
                row,
                0U,
                list.groundItems[i].order,
                list.groundItems[i].objectId,
                insertionOrder++ });
        }
        for (std::size_t i = 0; i < list.creatures.size(); ++i)
        {
            const auto& creature = list.creatures[i];
            std::int32_t row = static_cast<std::int32_t>(session.player.y);
            std::int32_t pixelY = static_cast<std::int32_t>(std::lround(
                session.player.motion.visualY * static_cast<float>(list.camera.projection.tileHeight)));
            std::uint32_t objectId = creature.objectId;
            if (!creature.localPlayer)
            {
                if (const auto found = session.world.objects.find(creature.objectId); found != session.world.objects.end())
                {
                    row = static_cast<std::int32_t>(found->second.y);
                    pixelY = static_cast<std::int32_t>(std::lround(
                        found->second.motion.visualY * static_cast<float>(list.camera.projection.tileHeight)));
                }
                else if (const auto fake = session.world.fakeCreatures.find(creature.objectId); fake != session.world.fakeCreatures.end())
                {
                    row = static_cast<std::int32_t>(fake->second.y);
                    pixelY = static_cast<std::int32_t>(std::lround(
                        fake->second.motion.visualY * static_cast<float>(list.camera.projection.tileHeight)));
                }
                else if (creature.initialNpcSeed && creature.npcSeedIndex < session.world.initialNpcSeeds.size())
                {
                    const auto& seed = session.world.initialNpcSeeds[creature.npcSeedIndex];
                    row = static_cast<std::int32_t>(seed.y);
                    pixelY = row * list.camera.projection.tileHeight;
                    objectId = static_cast<std::uint32_t>(creature.npcSeedIndex);
                }
            }
            list.layers.push_back(WorldLayerDrawItem{
                WorldLayerDrawKind::Creature,
                i,
                creature.order,
                WorldLayerPass::Row,
                row,
                2U,
                pixelY,
                objectId,
                insertionOrder++ });
        }
        for (std::size_t i = 0; i < list.effects.size(); ++i)
        {
            list.layers.push_back(WorldLayerDrawItem{
                WorldLayerDrawKind::Effect,
                i,
                list.effects[i].order,
                WorldLayerPass::Row,
                list.effects[i].order / std::max(1, list.camera.projection.tileHeight),
                3U,
                list.effects[i].order,
                static_cast<std::uint32_t>(list.effects[i].runtimeId),
                insertionOrder++ });
        }
        const auto layerSortStart = std::chrono::steady_clock::now();
        std::stable_sort(list.layers.begin(), list.layers.end(), [](const auto& left, const auto& right) {
            return std::tie(left.pass, left.legacyRow, left.legacyPriority, left.legacyPrimary, left.legacyObjectId) <
                std::tie(right.pass, right.legacyRow, right.legacyPriority, right.legacyPrimary, right.legacyObjectId);
        });
        list.layerSortMicroseconds = static_cast<std::uint64_t>(std::max<std::int64_t>(0,
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - layerSortStart).count()));

        return list;
    }
}