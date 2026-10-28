#pragma once

#include "Core/StrongId.h"
#include "Render/RenderTypes.h"

#include <cstdint>
#include <vector>

namespace phoenix::world
{
    struct TileCoord
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::int32_t z{ 0 };
    };

    struct TileVisual
    {
        TileCoord coord;
        core::SpriteId spriteId;
        std::int32_t elevation{ 0 };
    };

    struct MapObjectVisual
    {
        TileCoord coord;
        core::SpriteId spriteId;
        bool blocksMovement{ false };
        bool castsShadow{ false };
    };

    struct WorldRenderSnapshot
    {
        std::vector<TileVisual> tiles;
        std::vector<MapObjectVisual> objects;
    };

    struct IsometricProjection
    {
        std::int32_t tileWidth{ 48 };
        std::int32_t tileHeight{ 24 };
    };

    [[nodiscard]] constexpr render::ScreenPoint ProjectIsometric(
        const std::int32_t x,
        const std::int32_t y,
        const render::ScreenPoint origin,
        const IsometricProjection projection = {}) noexcept
    {
        return render::ScreenPoint{
            origin.x + (x * projection.tileWidth),
            origin.y + (y * projection.tileHeight)
        };
    }

    class IWorldRenderSource
    {
    public:
        virtual ~IWorldRenderSource() = default;
        [[nodiscard]] virtual std::vector<render::RenderCommand> BuildRenderCommands(
            const WorldRenderSnapshot& snapshot) const = 0;
    };
}
