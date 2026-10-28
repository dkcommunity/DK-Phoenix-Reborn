#pragma once

#include "World/LegacyMapReader.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace phoenix::tools::map_editor
{
    enum class PropertyField
    {
        ReadOnly,
        MapName,
        ZoneId,
        ZoneGroupId,
        ZoneType,
        ZoneLevel,
        Description,
        TileSpriteId,
        TileProperty,
        TileLight,
        BlockUnderground,
        BlockGround,
        BlockFlying,
        SafetyCommon,
        SafetySlayer,
        SafetyVampire,
        SafetyOusters,
        PortalEnabled,
        PortalType,
        PortalTargets,
        ObjectType,
        WrapperType,
        SpriteId,
        WallDirection,
        WallValue,
        PixelX,
        PixelY,
        PositionX,
        PositionY,
        Viewpoint,
        Animation,
        Transparent,
        FrameId,
        MaxFrame,
        BltType,
        Direction,
        SoundFrame,
        SoundId,
        Loop,
        MinDelay,
        MaxDelay,
        StartHour,
        EndHour,
        InteractionType
    };

    struct PropertyTarget
    {
        std::optional<std::pair<std::uint16_t, std::uint16_t>> tile;
        std::optional<std::uint32_t> objectId;
        std::optional<std::size_t> positionIndex;
    };

    struct PropertyEditResult
    {
        bool applied{ false };
        bool tileResourceChanged{ false };
        bool objectResourceChanged{ false };
        std::uint32_t resourceSpriteId{ 0xFFFFFFFFU };
        std::string error;
    };

    struct MapSelectionRect
    {
        std::uint16_t left{ 0 };
        std::uint16_t top{ 0 };
        std::uint16_t right{ 0 };
        std::uint16_t bottom{ 0 };

        [[nodiscard]] bool IsValid() const noexcept { return left <= right && top <= bottom; }
    };

    class MapEditHistory final
    {
    public:
        void Clear() noexcept;
        void Begin(const world::LegacyMap& map);
        [[nodiscard]] bool Undo(world::LegacyMap& map);
        [[nodiscard]] bool Redo(world::LegacyMap& map);
        [[nodiscard]] bool CanUndo() const noexcept;
        [[nodiscard]] bool CanRedo() const noexcept;

    private:
        struct Snapshot
        {
            std::uint16_t zoneId{ 0 };
            std::uint16_t zoneGroupId{ 0 };
            std::string name;
            std::uint8_t zoneType{ 0 };
            std::uint8_t zoneLevel{ 0 };
            std::string description;
            bool modified{ false };
            std::vector<world::LegacyMapSector> sectors;
            std::vector<world::LegacyMapImageObject> objects;
            std::vector<world::LegacyMapSpriteFilePosition> tileResources;
            std::vector<world::LegacyMapSpriteFilePosition> objectResources;
            world::LegacyMapSidecarData sidecars;
        };

        [[nodiscard]] static Snapshot Capture(const world::LegacyMap& map);
        static void Restore(world::LegacyMap& map, const Snapshot& snapshot);

        std::vector<Snapshot> undo_;
        std::vector<Snapshot> redo_;
    };

    class LegacyMapPropertyEditor final
    {
    public:
        [[nodiscard]] static PropertyEditResult Apply(
            world::LegacyMap& map,
            MapEditHistory& history,
            PropertyField field,
            const std::string& value,
            const PropertyTarget& target);
    };

    void RebuildObjectIndex(world::LegacyMap& map);
    [[nodiscard]] bool CanMoveObject(const world::LegacyMap& map, std::size_t objectIndex,
        int deltaX, int deltaY) noexcept;
    [[nodiscard]] bool MoveObject(world::LegacyMap& map, std::size_t objectIndex,
        int deltaX, int deltaY);
    [[nodiscard]] bool CanMoveMapSelection(const world::LegacyMap& map,
        const MapSelectionRect& tiles, const std::set<std::uint32_t>& objectIds,
        int deltaX, int deltaY) noexcept;
    [[nodiscard]] bool MoveMapSelection(world::LegacyMap& map,
        const MapSelectionRect& tiles, const std::set<std::uint32_t>& objectIds,
        int deltaX, int deltaY);
    [[nodiscard]] bool CanDeleteMapSelection(const world::LegacyMap& map,
        const std::optional<MapSelectionRect>& tiles,
        const std::set<std::uint32_t>& objectIds) noexcept;
    [[nodiscard]] bool DeleteMapSelection(world::LegacyMap& map,
        const std::optional<MapSelectionRect>& tiles,
        const std::set<std::uint32_t>& objectIds);
    // Sidecar portal kaydini siler; tile sprite / light / collision dokunulmaz.
    [[nodiscard]] bool ClearPortalOnTile(world::LegacyMap& map, MapEditHistory& history,
        std::uint16_t x, std::uint16_t y);

    struct EdgePortalFillParams
    {
        MapSelectionRect rect;
        std::uint8_t type{ 0 };
        std::uint16_t destZoneId{ 0 };
        std::uint8_t destStartX{ 0 };
        std::uint8_t destStartY{ 0 };
        std::uint8_t destEndX{ 0 };
        std::uint8_t destEndY{ 0 };
    };

    struct EdgePortalFillResult
    {
        bool applied{ false };
        std::uint32_t tilesWritten{ 0 };
        std::string error;
    };

    [[nodiscard]] std::uint32_t CountPortalsInRect(const world::LegacyMap& map, const MapSelectionRect& rect) noexcept;
    [[nodiscard]] std::string DescribeEdgePortalFill(const EdgePortalFillParams& params);
    // Kenar seridi: uzun eksende dest lerp, kalinlik ayni dest'i kopyalar.
    // Type 3/4 yok. Sprite / collision dokunulmaz. Tek history.Begin.
    [[nodiscard]] EdgePortalFillResult FillEdgePortals(world::LegacyMap& map, MapEditHistory& history,
        const EdgePortalFillParams& params);
    [[nodiscard]] bool ClearPortalsInRect(world::LegacyMap& map, MapEditHistory& history,
        const MapSelectionRect& rect);
}
