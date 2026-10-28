#include "MapEditorDocument.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <string_view>

namespace phoenix::tools::map_editor
{
    namespace
    {
        std::optional<std::int64_t> ParseInteger(const std::string& value)
        {
            if (value.empty()) return std::nullopt;
            std::int64_t parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return std::nullopt;
            return parsed;
        }

        std::optional<bool> ParseBoolean(const std::string& value)
        {
            if (value == "1" || value == "true" || value == "TRUE") return true;
            if (value == "0" || value == "false" || value == "FALSE") return false;
            return std::nullopt;
        }

        world::LegacyMapImageObject* FindObject(world::LegacyMap& map, const std::optional<std::uint32_t> objectId)
        {
            if (!objectId) return nullptr;
            const auto found = std::find_if(map.imageObjects.begin(), map.imageObjects.end(),
                [objectId](const auto& object) { return object.objectId == *objectId; });
            return found == map.imageObjects.end() ? nullptr : &*found;
        }

        world::LegacyMapSector* FindTile(world::LegacyMap& map,
            const std::optional<std::pair<std::uint16_t, std::uint16_t>>& coordinate)
        {
            if (!coordinate || coordinate->first >= map.width || coordinate->second >= map.height) return nullptr;
            return &map.sectors[static_cast<std::size_t>(coordinate->second) * map.width + coordinate->first];
        }

        bool InRange(const std::optional<std::int64_t> value, const std::int64_t minimum, const std::int64_t maximum)
        {
            return value && *value >= minimum && *value <= maximum;
        }

        PropertyEditResult Error(std::string message)
        {
            PropertyEditResult result;
            result.error = std::move(message);
            return result;
        }

        std::optional<std::vector<world::LegacyMapPortalTarget>> ParsePortalTargets(const std::string& value)
        {
            if (value.empty()) return std::nullopt;
            std::vector<world::LegacyMapPortalTarget> targets;
            std::size_t begin = 0;
            while (begin < value.size())
            {
                const auto end = value.find(';', begin);
                const std::string_view item(value.data() + begin,
                    (end == std::string::npos ? value.size() : end) - begin);
                const auto first = item.find(':');
                const auto second = first == std::string_view::npos ? std::string_view::npos : item.find(':', first + 1U);
                if (first == std::string_view::npos || second == std::string_view::npos ||
                    item.find(':', second + 1U) != std::string_view::npos) return std::nullopt;
                const auto parse = [](const std::string_view text) -> std::optional<std::uint32_t> {
                    std::uint32_t parsed = 0;
                    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
                    if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size()) return std::nullopt;
                    return parsed;
                };
                const auto zone = parse(item.substr(0, first));
                const auto x = parse(item.substr(first + 1U, second - first - 1U));
                const auto y = parse(item.substr(second + 1U));
                if (!zone || !x || !y || *zone > 65535U || *x > 255U || *y > 255U) return std::nullopt;
                targets.push_back({ static_cast<std::uint16_t>(*zone), static_cast<std::uint8_t>(*x),
                    static_cast<std::uint8_t>(*y), true });
                if (targets.size() > 255U) return std::nullopt;
                if (end == std::string::npos) break;
                begin = end + 1U;
            }
            return targets.empty() ? std::nullopt : std::optional{ std::move(targets) };
        }

        bool IsPortalType(const std::int64_t value) noexcept
        {
            return (value >= 0 && value <= 7) || (value >= 200 && value <= 203) || value == 255;
        }
    }

    MapEditHistory::Snapshot MapEditHistory::Capture(const world::LegacyMap& map)
    {
        return { map.zoneId, map.zoneGroupId, map.name, map.zoneType, map.zoneLevel, map.description,
            map.modified, map.sectors, map.imageObjects, map.tileSpriteFilePositions,
            map.imageObjectSpriteFilePositions, map.sidecars };
    }

    void MapEditHistory::Restore(world::LegacyMap& map, const Snapshot& snapshot)
    {
        map.zoneId = snapshot.zoneId;
        map.zoneGroupId = snapshot.zoneGroupId;
        map.name = snapshot.name;
        map.zoneType = snapshot.zoneType;
        map.zoneLevel = snapshot.zoneLevel;
        map.description = snapshot.description;
        map.modified = snapshot.modified;
        map.sectors = snapshot.sectors;
        map.imageObjects = snapshot.objects;
        map.tileSpriteFilePositions = snapshot.tileResources;
        map.imageObjectSpriteFilePositions = snapshot.objectResources;
        map.sidecars = snapshot.sidecars;
        RebuildObjectIndex(map);
    }

    void MapEditHistory::Clear() noexcept
    {
        undo_.clear();
        redo_.clear();
    }

    void MapEditHistory::Begin(const world::LegacyMap& map)
    {
        undo_.push_back(Capture(map));
        if (undo_.size() > 64U) undo_.erase(undo_.begin());
        redo_.clear();
    }

    bool MapEditHistory::Undo(world::LegacyMap& map)
    {
        if (undo_.empty()) return false;
        redo_.push_back(Capture(map));
        auto snapshot = std::move(undo_.back());
        undo_.pop_back();
        Restore(map, snapshot);
        return true;
    }

    bool MapEditHistory::Redo(world::LegacyMap& map)
    {
        if (redo_.empty()) return false;
        undo_.push_back(Capture(map));
        auto snapshot = std::move(redo_.back());
        redo_.pop_back();
        Restore(map, snapshot);
        return true;
    }

    bool MapEditHistory::CanUndo() const noexcept { return !undo_.empty(); }
    bool MapEditHistory::CanRedo() const noexcept { return !redo_.empty(); }

    PropertyEditResult LegacyMapPropertyEditor::Apply(
        world::LegacyMap& map,
        MapEditHistory& history,
        const PropertyField field,
        const std::string& value,
        const PropertyTarget& target)
    {
        auto* tile = FindTile(map, target.tile);
        auto* object = FindObject(map, target.objectId);
        const auto integer = ParseInteger(value);
        const auto boolean = ParseBoolean(value);

        switch (field)
        {
        case PropertyField::ReadOnly: return Error("The selected property is read-only.");
        case PropertyField::TileSpriteId:
        case PropertyField::TileProperty:
        case PropertyField::TileLight:
        case PropertyField::BlockUnderground:
        case PropertyField::BlockGround:
        case PropertyField::BlockFlying:
        case PropertyField::SafetyCommon:
        case PropertyField::SafetySlayer:
        case PropertyField::SafetyVampire:
        case PropertyField::SafetyOusters:
        case PropertyField::PortalEnabled:
        case PropertyField::PortalType:
        case PropertyField::PortalTargets:
            if (tile == nullptr) return Error("No valid tile is selected.");
            break;
        case PropertyField::ObjectType:
        case PropertyField::WrapperType:
        case PropertyField::SpriteId:
        case PropertyField::WallDirection:
        case PropertyField::WallValue:
        case PropertyField::PixelX:
        case PropertyField::PixelY:
        case PropertyField::PositionX:
        case PropertyField::PositionY:
        case PropertyField::Viewpoint:
        case PropertyField::Animation:
        case PropertyField::Transparent:
        case PropertyField::FrameId:
        case PropertyField::MaxFrame:
        case PropertyField::BltType:
        case PropertyField::Direction:
        case PropertyField::SoundFrame:
        case PropertyField::SoundId:
        case PropertyField::Loop:
        case PropertyField::MinDelay:
        case PropertyField::MaxDelay:
        case PropertyField::StartHour:
        case PropertyField::EndHour:
        case PropertyField::InteractionType:
            if (object == nullptr) return Error("No valid map object is selected.");
            break;
        default: break;
        }

        const bool animationField = field == PropertyField::FrameId || field == PropertyField::MaxFrame ||
            field == PropertyField::BltType || field == PropertyField::Direction || field == PropertyField::SoundFrame ||
            field == PropertyField::SoundId || field == PropertyField::Loop || field == PropertyField::MinDelay ||
            field == PropertyField::MaxDelay || field == PropertyField::StartHour || field == PropertyField::EndHour;
        if (animationField && object != nullptr && object->wrapperType != 5U && object->wrapperType != 6U && object->wrapperType != 7U)
            return Error("The selected Image Object does not serialize Animation Object fields.");
        if (field == PropertyField::InteractionType && object != nullptr && object->wrapperType != 7U)
            return Error("Interaction type is serialized only by Legacy wrapper type 7.");

        if ((field == PropertyField::Animation || field == PropertyField::Transparent || field == PropertyField::Loop ||
             field == PropertyField::BlockUnderground || field == PropertyField::BlockGround ||
             field == PropertyField::BlockFlying || field == PropertyField::SafetyCommon ||
             field == PropertyField::SafetySlayer || field == PropertyField::SafetyVampire ||
             field == PropertyField::SafetyOusters || field == PropertyField::PortalEnabled) && !boolean)
            return Error("Expected a boolean value: 0, 1, false or true.");
        if ((field == PropertyField::ZoneId || field == PropertyField::ZoneGroupId ||
             field == PropertyField::SpriteId ||
             field == PropertyField::WallDirection || field == PropertyField::WallValue ||
             field == PropertyField::Viewpoint || field == PropertyField::FrameId ||
             field == PropertyField::SoundId || field == PropertyField::InteractionType) &&
            !InRange(integer, 0, std::numeric_limits<std::uint16_t>::max()))
            return Error("Value is outside the Legacy uint16 range.");
        const auto maximumTileSpriteId = map.parserVariant == world::LegacyMapParserVariant::Map2024LayoutCompatible
            ? static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())
            : static_cast<std::uint64_t>(std::numeric_limits<std::uint16_t>::max());
        if (field == PropertyField::TileSpriteId && !InRange(integer, 0, maximumTileSpriteId))
            return Error("Tile sprite ID is outside the selected Legacy map variant range.");
        if ((field == PropertyField::ZoneType || field == PropertyField::ZoneLevel ||
             field == PropertyField::TileProperty || field == PropertyField::TileLight ||
             field == PropertyField::ObjectType || field == PropertyField::MaxFrame ||
             field == PropertyField::SoundFrame) && !InRange(integer, 0, std::numeric_limits<std::uint8_t>::max()))
            return Error("Value is outside the Legacy byte range.");
        if (field == PropertyField::WrapperType && !InRange(integer, 3, 7))
            return Error("Wrapper type must be in the supported Legacy range 3..7.");
        if (field == PropertyField::BltType && !InRange(integer, 0, 3))
            return Error("Blt type must be in the Legacy range 0..3.");
        if (field == PropertyField::Direction && !InRange(integer, 0, 7))
            return Error("Direction must be in the Legacy range 0..7.");
        if (field == PropertyField::PortalType && (!integer || !IsPortalType(*integer)))
            return Error("Portal type must be 0-7, 200-203 or 255.");
        if ((field == PropertyField::StartHour || field == PropertyField::EndHour) && !InRange(integer, 0, 23))
            return Error("Hour must be in the range 0..23.");
        if ((field == PropertyField::PixelX || field == PropertyField::PixelY) &&
            !InRange(integer, std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max()))
            return Error("Value is outside the Legacy int32 range.");
        if ((field == PropertyField::PositionX || field == PropertyField::PositionY) &&
            (!target.positionIndex || *target.positionIndex >= object->positions.size()))
            return Error("The selected object sector-position record is invalid.");
        if (field == PropertyField::PositionX && !InRange(integer, 0, map.width - 1U))
            return Error("Sector X is outside the map bounds.");
        if (field == PropertyField::PositionY && !InRange(integer, 0, map.height - 1U))
            return Error("Sector Y is outside the map bounds.");
        if ((field == PropertyField::MinDelay || field == PropertyField::MaxDelay) &&
            !InRange(integer, 0, std::numeric_limits<std::uint32_t>::max()))
            return Error("Value is outside the Legacy uint32 range.");
        if (field == PropertyField::MinDelay && static_cast<std::uint32_t>(*integer) > object->maxDelay)
            return Error("Minimum delay cannot exceed maximum delay.");
        if (field == PropertyField::MaxDelay && static_cast<std::uint32_t>(*integer) < object->minDelay)
            return Error("Maximum delay cannot be less than minimum delay.");

        auto* server = target.tile ? world::LegacyServerSector(map, target.tile->first, target.tile->second) : nullptr;
        const bool serverField = field == PropertyField::SafetyCommon || field == PropertyField::SafetySlayer ||
            field == PropertyField::SafetyVampire || field == PropertyField::SafetyOusters ||
            field == PropertyField::PortalEnabled || field == PropertyField::PortalType || field == PropertyField::PortalTargets;
        if (serverField && server == nullptr) return Error("Server sidecar data is not initialized for the selected tile.");
        if (field == PropertyField::PortalEnabled && *boolean && !server->portal)
            return Error("Enter Portal targets as zone:x:y first; an incomplete portal cannot be enabled.");
        if (field == PropertyField::PortalType && server->portal && server->portal->targets.size() > 1U &&
            *integer != 3 && *integer != 4)
            return Error("A portal with multiple targets must remain type 3 or 4.");
        const auto portalTargets = field == PropertyField::PortalTargets ? ParsePortalTargets(value) : std::nullopt;
        if (field == PropertyField::PortalTargets && !portalTargets)
            return Error("Portal targets must use zone:x:y;zone:x:y format and Legacy numeric ranges.");
        if (field == PropertyField::PortalTargets && server->portal)
        {
            const bool multi = server->portal->type == 3U || server->portal->type == 4U;
            if (!multi && portalTargets->size() != 1U)
                return Error("Only portal types 3 and 4 may contain multiple targets.");
        }

        history.Begin(map);
        PropertyEditResult result;
        result.applied = true;
        if (field == PropertyField::MapName || field == PropertyField::ZoneId || field == PropertyField::ZoneGroupId ||
            field == PropertyField::ZoneType || field == PropertyField::ZoneLevel || field == PropertyField::Description)
            map.sidecars.modified = true;
        switch (field)
        {
        case PropertyField::MapName: map.name = value; break;
        case PropertyField::ZoneId: map.zoneId = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::ZoneGroupId: map.zoneGroupId = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::ZoneType: map.zoneType = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::ZoneLevel: map.zoneLevel = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::Description: map.description = value; break;
        case PropertyField::TileSpriteId:
            tile->spriteId = static_cast<std::uint32_t>(*integer); result.tileResourceChanged = true; result.resourceSpriteId = tile->spriteId; break;
        case PropertyField::TileProperty:
            if ((tile->property & 0x07U) != (static_cast<std::uint8_t>(*integer) & 0x07U)) map.sidecars.modified = true;
            tile->property = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::TileLight: tile->light = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::BlockUnderground:
        case PropertyField::BlockGround:
        case PropertyField::BlockFlying:
        {
            const auto flag = field == PropertyField::BlockUnderground ? world::kLegacyBlockUnderground :
                field == PropertyField::BlockGround ? world::kLegacyBlockGround : world::kLegacyBlockFlying;
            if (*boolean) tile->property |= flag; else tile->property &= static_cast<std::uint8_t>(~flag);
            map.sidecars.modified = true;
            break;
        }
        case PropertyField::SafetyCommon:
        case PropertyField::SafetySlayer:
        case PropertyField::SafetyVampire:
        case PropertyField::SafetyOusters:
        {
            const auto flag = field == PropertyField::SafetyCommon ? world::kLegacySafetyCommon :
                field == PropertyField::SafetySlayer ? world::kLegacySafetySlayer :
                field == PropertyField::SafetyVampire ? world::kLegacySafetyVampire : world::kLegacySafetyOusters;
            if (*boolean) server->safetyFlags |= flag; else server->safetyFlags &= static_cast<std::uint8_t>(~flag);
            map.sidecars.modified = true;
            break;
        }
        case PropertyField::PortalEnabled:
            if (!*boolean) server->portal.reset();
            map.sidecars.modified = true;
            break;
        case PropertyField::PortalType:
            if (!server->portal) server->portal = world::LegacyMapPortal{};
            server->portal->type = static_cast<std::uint8_t>(*integer); map.sidecars.modified = true; break;
        case PropertyField::PortalTargets:
            if (!server->portal) server->portal = world::LegacyMapPortal{};
            server->portal->targets = *portalTargets; map.sidecars.modified = true; break;
        case PropertyField::ObjectType: object->objectType = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::WrapperType:
            object->wrapperType = static_cast<std::uint8_t>(*integer);
            object->animation = object->wrapperType == 5U || object->wrapperType == 6U || object->wrapperType == 7U;
            break;
        case PropertyField::SpriteId:
            object->spriteId = static_cast<std::uint16_t>(*integer); result.objectResourceChanged = true; result.resourceSpriteId = object->spriteId; break;
        case PropertyField::WallDirection: object->x = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::WallValue: object->y = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::PixelX: object->pixelX = static_cast<std::int32_t>(*integer); break;
        case PropertyField::PixelY: object->pixelY = static_cast<std::int32_t>(*integer); break;
        case PropertyField::PositionX: object->positions[*target.positionIndex].first = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::PositionY: object->positions[*target.positionIndex].second = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::Viewpoint: object->viewpoint = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::Animation: object->animation = *boolean; break;
        case PropertyField::Transparent: object->transparent = *boolean; break;
        case PropertyField::FrameId: object->frameId = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::MaxFrame: object->maxFrame = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::BltType: object->bltType = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::Direction: object->direction = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::SoundFrame: object->soundFrame = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::SoundId: object->soundId = static_cast<std::uint16_t>(*integer); break;
        case PropertyField::Loop: object->loop = *boolean; break;
        case PropertyField::MinDelay: object->minDelay = static_cast<std::uint32_t>(*integer); break;
        case PropertyField::MaxDelay: object->maxDelay = static_cast<std::uint32_t>(*integer); break;
        case PropertyField::StartHour: object->startHour = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::EndHour: object->endHour = static_cast<std::uint8_t>(*integer); break;
        case PropertyField::InteractionType: object->interactionType = static_cast<std::uint16_t>(*integer); break;
        }
        map.modified = true;
        return result;
    }

    void RebuildObjectIndex(world::LegacyMap& map)
    {
        map.sectorImageObjectIndices.assign(static_cast<std::size_t>(map.width) * map.height, {});
        for (std::size_t index = 0; index < map.imageObjects.size(); ++index)
        {
            for (const auto& [x, y] : map.imageObjects[index].positions)
            {
                if (x < map.width && y < map.height)
                    map.sectorImageObjectIndices[static_cast<std::size_t>(y) * map.width + x].push_back(index);
            }
        }
    }

    bool CanMoveObject(const world::LegacyMap& map, const std::size_t objectIndex,
        const int deltaX, const int deltaY) noexcept
    {
        if (objectIndex >= map.imageObjects.size() || map.width == 0U || map.height == 0U ||
            (deltaX == 0 && deltaY == 0)) return false;
        const auto& object = map.imageObjects[objectIndex];
        if (object.positions.empty()) return false;
        const auto& position = object.positions.front();
        const auto actualX = std::clamp(deltaX, -static_cast<int>(position.first),
            static_cast<int>(map.width - 1U - position.first));
        const auto actualY = std::clamp(deltaY, -static_cast<int>(position.second),
            static_cast<int>(map.height - 1U - position.second));
        return actualX != 0 || actualY != 0;
    }

    bool MoveObject(world::LegacyMap& map, const std::size_t objectIndex,
        const int deltaX, const int deltaY)
    {
        if (!CanMoveObject(map, objectIndex, deltaX, deltaY)) return false;
        auto& object = map.imageObjects[objectIndex];
        const auto removeFromSectors = [&]() {
            for (const auto& position : object.positions)
            {
                if (position.first >= map.width || position.second >= map.height) continue;
                const auto sectorIndex = static_cast<std::size_t>(position.second) * map.width + position.first;
                if (sectorIndex >= map.sectorImageObjectIndices.size()) continue;
                auto& indices = map.sectorImageObjectIndices[sectorIndex];
                indices.erase(std::remove(indices.begin(), indices.end(), objectIndex), indices.end());
            }
        };
        const auto addToSectors = [&]() {
            for (const auto& position : object.positions)
            {
                if (position.first >= map.width || position.second >= map.height) continue;
                const auto sectorIndex = static_cast<std::size_t>(position.second) * map.width + position.first;
                if (sectorIndex >= map.sectorImageObjectIndices.size()) continue;
                auto& indices = map.sectorImageObjectIndices[sectorIndex];
                const auto insertion = std::lower_bound(indices.begin(), indices.end(), objectIndex);
                if (insertion == indices.end() || *insertion != objectIndex) indices.insert(insertion, objectIndex);
            }
        };

        removeFromSectors();
        const auto actualX = std::clamp(deltaX, -static_cast<int>(object.positions.front().first),
            static_cast<int>(map.width - 1U - object.positions.front().first));
        const auto actualY = std::clamp(deltaY, -static_cast<int>(object.positions.front().second),
            static_cast<int>(map.height - 1U - object.positions.front().second));
        object.pixelX += actualX * 48;
        object.pixelY += actualY * 24;
        object.viewpoint = static_cast<std::uint16_t>(
            std::clamp(static_cast<int>(object.viewpoint) + actualY, 0, 65535));
        for (auto& position : object.positions)
        {
            position.first = static_cast<std::uint16_t>(std::clamp(
                static_cast<int>(position.first) + actualX, 0, static_cast<int>(map.width) - 1));
            position.second = static_cast<std::uint16_t>(std::clamp(
                static_cast<int>(position.second) + actualY, 0, static_cast<int>(map.height) - 1));
        }
        addToSectors();
        return true;
    }

    bool CanMoveMapSelection(const world::LegacyMap& map, const MapSelectionRect& tiles,
        const std::set<std::uint32_t>& objectIds, const int deltaX, const int deltaY) noexcept
    {
        if (!tiles.IsValid() || map.width == 0U || map.height == 0U ||
            map.sectors.size() != static_cast<std::size_t>(map.width) * map.height ||
            tiles.right >= map.width || tiles.bottom >= map.height || (deltaX == 0 && deltaY == 0)) return false;
        const auto targetLeft = static_cast<std::int64_t>(tiles.left) + deltaX;
        const auto targetTop = static_cast<std::int64_t>(tiles.top) + deltaY;
        const auto targetRight = static_cast<std::int64_t>(tiles.right) + deltaX;
        const auto targetBottom = static_cast<std::int64_t>(tiles.bottom) + deltaY;
        if (targetLeft < 0 || targetTop < 0 || targetRight >= map.width || targetBottom >= map.height) return false;

        for (const auto objectId : objectIds)
        {
            const auto found = std::find_if(map.imageObjects.begin(), map.imageObjects.end(),
                [objectId](const auto& object) { return object.objectId == objectId; });
            if (found == map.imageObjects.end()) return false;
            for (const auto& position : found->positions)
            {
                const auto x = static_cast<std::int64_t>(position.first) + deltaX;
                const auto y = static_cast<std::int64_t>(position.second) + deltaY;
                if (x < 0 || y < 0 || x >= map.width || y >= map.height) return false;
            }
            const auto pixelX = static_cast<std::int64_t>(found->pixelX) + static_cast<std::int64_t>(deltaX) * 48;
            const auto pixelY = static_cast<std::int64_t>(found->pixelY) + static_cast<std::int64_t>(deltaY) * 24;
            if (pixelX < std::numeric_limits<std::int32_t>::min() || pixelX > std::numeric_limits<std::int32_t>::max() ||
                pixelY < std::numeric_limits<std::int32_t>::min() || pixelY > std::numeric_limits<std::int32_t>::max()) return false;
            if (found->viewpoint != 0U)
            {
                const auto viewpoint = static_cast<std::int64_t>(found->viewpoint) + deltaY;
                if (viewpoint <= 0 || viewpoint > std::numeric_limits<std::uint16_t>::max()) return false;
            }
        }
        return true;
    }

    bool MoveMapSelection(world::LegacyMap& map, const MapSelectionRect& tiles,
        const std::set<std::uint32_t>& objectIds, const int deltaX, const int deltaY)
    {
        if (!CanMoveMapSelection(map, tiles, objectIds, deltaX, deltaY)) return false;
        auto movedBounds = tiles;
        const auto swapTiles = [&](const int x1, const int y1, const int x2, const int y2) {
            std::swap(map.sectors[static_cast<std::size_t>(y1) * map.width + x1],
                map.sectors[static_cast<std::size_t>(y2) * map.width + x2]);
            if (map.sidecars.sectors.size() == map.sectors.size())
                std::swap(map.sidecars.sectors[static_cast<std::size_t>(y1) * map.width + x1],
                    map.sidecars.sectors[static_cast<std::size_t>(y2) * map.width + x2]);
        };
        for (int step = 0; step < std::abs(deltaX); ++step)
        {
            if (deltaX > 0)
            {
                for (int y = movedBounds.top; y <= movedBounds.bottom; ++y)
                    for (int x = movedBounds.right; x >= movedBounds.left; --x) swapTiles(x, y, x + 1, y);
                ++movedBounds.left; ++movedBounds.right;
            }
            else
            {
                for (int y = movedBounds.top; y <= movedBounds.bottom; ++y)
                    for (int x = movedBounds.left; x <= movedBounds.right; ++x) swapTiles(x, y, x - 1, y);
                --movedBounds.left; --movedBounds.right;
            }
        }
        for (int step = 0; step < std::abs(deltaY); ++step)
        {
            if (deltaY > 0)
            {
                for (int y = movedBounds.bottom; y >= movedBounds.top; --y)
                    for (int x = movedBounds.left; x <= movedBounds.right; ++x) swapTiles(x, y, x, y + 1);
                ++movedBounds.top; ++movedBounds.bottom;
            }
            else
            {
                for (int y = movedBounds.top; y <= movedBounds.bottom; ++y)
                    for (int x = movedBounds.left; x <= movedBounds.right; ++x) swapTiles(x, y, x, y - 1);
                --movedBounds.top; --movedBounds.bottom;
            }
        }

        for (auto& object : map.imageObjects)
        {
            if (objectIds.count(object.objectId) == 0U) continue;
            object.pixelX += deltaX * 48;
            object.pixelY += deltaY * 24;
            if (object.viewpoint != 0U)
                object.viewpoint = static_cast<std::uint16_t>(static_cast<int>(object.viewpoint) + deltaY);
            for (auto& position : object.positions)
            {
                position.first = static_cast<std::uint16_t>(static_cast<int>(position.first) + deltaX);
                position.second = static_cast<std::uint16_t>(static_cast<int>(position.second) + deltaY);
            }
        }
        RebuildObjectIndex(map);
        map.sidecars.modified = true;
        map.modified = true;
        return true;
    }

    bool CanDeleteMapSelection(const world::LegacyMap& map,
        const std::optional<MapSelectionRect>& tiles,
        const std::set<std::uint32_t>& objectIds) noexcept
    {
        if (tiles)
        {
            if (!tiles->IsValid() || tiles->right >= map.width || tiles->bottom >= map.height ||
                map.sectors.size() != static_cast<std::size_t>(map.width) * map.height) return false;
            for (std::uint32_t y = tiles->top; y <= tiles->bottom; ++y)
            {
                for (std::uint32_t x = tiles->left; x <= tiles->right; ++x)
                {
                    if (map.sectors[static_cast<std::size_t>(y) * map.width + x].spriteId != 0xFFFFU) return true;
                }
            }
        }
        return std::any_of(map.imageObjects.begin(), map.imageObjects.end(), [&](const auto& object) {
            return objectIds.count(object.objectId) != 0U;
        });
    }

    bool DeleteMapSelection(world::LegacyMap& map,
        const std::optional<MapSelectionRect>& tiles,
        const std::set<std::uint32_t>& objectIds)
    {
        if (!CanDeleteMapSelection(map, tiles, objectIds)) return false;
        if (tiles)
        {
            for (std::uint32_t y = tiles->top; y <= tiles->bottom; ++y)
            {
                for (std::uint32_t x = tiles->left; x <= tiles->right; ++x)
                    map.sectors[static_cast<std::size_t>(y) * map.width + x].spriteId = 0xFFFFU;
            }
        }
        map.imageObjects.erase(std::remove_if(map.imageObjects.begin(), map.imageObjects.end(),
            [&](const auto& object) { return objectIds.count(object.objectId) != 0U; }), map.imageObjects.end());
        RebuildObjectIndex(map);
        map.modified = true;
        return true;
    }

    bool ClearPortalOnTile(world::LegacyMap& map, MapEditHistory& history,
        const std::uint16_t x, const std::uint16_t y)
    {
        auto* server = world::LegacyServerSector(map, x, y);
        if (server == nullptr || !server->portal) return false;
        history.Begin(map);
        server->portal.reset();
        map.sidecars.modified = true;
        map.modified = true;
        return true;
    }

    namespace
    {
        bool PortalRectInBounds(const world::LegacyMap& map, const MapSelectionRect& rect) noexcept
        {
            return rect.IsValid() && rect.right < map.width && rect.bottom < map.height &&
                map.sidecars.sectors.size() == static_cast<std::size_t>(map.width) * map.height;
        }

        bool IsEdgeFillPortalType(const std::uint8_t type) noexcept
        {
            return type <= 7U && type != 3U && type != 4U;
        }

        std::uint8_t LerpByte(const std::uint8_t start, const std::uint8_t end,
            const std::uint32_t index, const std::uint32_t count) noexcept
        {
            if (count <= 1U) return start;
            const int delta = static_cast<int>(end) - static_cast<int>(start);
            return static_cast<std::uint8_t>(static_cast<int>(start) +
                delta * static_cast<int>(index) / static_cast<int>(count - 1U));
        }

        void EdgeFillAxis(const MapSelectionRect& rect, std::uint32_t& count, bool& alongX) noexcept
        {
            const auto width = static_cast<std::uint32_t>(rect.right - rect.left) + 1U;
            const auto height = static_cast<std::uint32_t>(rect.bottom - rect.top) + 1U;
            alongX = width >= height;
            count = alongX ? width : height;
        }

        world::LegacyMapPortal MakeFilledPortal(const EdgePortalFillParams& params,
            const std::uint16_t x, const std::uint16_t y)
        {
            std::uint32_t axisCount = 1U;
            bool alongX = true;
            EdgeFillAxis(params.rect, axisCount, alongX);
            const auto index = alongX
                ? static_cast<std::uint32_t>(x - params.rect.left)
                : static_cast<std::uint32_t>(y - params.rect.top);
            world::LegacyMapPortal portal;
            portal.type = params.type;
            portal.targets.push_back({
                params.destZoneId,
                LerpByte(params.destStartX, params.destEndX, index, axisCount),
                LerpByte(params.destStartY, params.destEndY, index, axisCount),
                true });
            return portal;
        }

        std::string ValidateEdgePortalFill(const world::LegacyMap& map, const EdgePortalFillParams& params)
        {
            if (!PortalRectInBounds(map, params.rect))
                return "Portal fill rectangle is outside the map or sidecar data is missing.";
            if (params.destZoneId == 0U)
                return "Destination Zone ID must be in the range 1..65535.";
            if (!IsEdgeFillPortalType(params.type))
                return "Edge fill supports portal types 0, 1, 2, 5, 6 and 7 (not multi-target 3/4).";
            return {};
        }
    }

    std::uint32_t CountPortalsInRect(const world::LegacyMap& map, const MapSelectionRect& rect) noexcept
    {
        if (!PortalRectInBounds(map, rect)) return 0U;
        std::uint32_t count = 0U;
        for (std::uint32_t y = rect.top; y <= rect.bottom; ++y)
        {
            for (std::uint32_t x = rect.left; x <= rect.right; ++x)
            {
                const auto* server = world::LegacyServerSector(map,
                    static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                if (server != nullptr && server->portal) ++count;
            }
        }
        return count;
    }

    std::string DescribeEdgePortalFill(const EdgePortalFillParams& params)
    {
        if (!params.rect.IsValid()) return "Invalid selection.";
        const auto width = static_cast<std::uint32_t>(params.rect.right - params.rect.left) + 1U;
        const auto height = static_cast<std::uint32_t>(params.rect.bottom - params.rect.top) + 1U;
        const auto tiles = width * height;
        std::uint32_t axisCount = 1U;
        bool alongX = true;
        EdgeFillAxis(params.rect, axisCount, alongX);
        const auto firstX = LerpByte(params.destStartX, params.destEndX, 0U, axisCount);
        const auto firstY = LerpByte(params.destStartY, params.destEndY, 0U, axisCount);
        const auto lastX = LerpByte(params.destStartX, params.destEndX, axisCount - 1U, axisCount);
        const auto lastY = LerpByte(params.destStartY, params.destEndY, axisCount - 1U, axisCount);
        std::string text = std::to_string(tiles) + " tiles -> " + std::to_string(params.destZoneId) + ":(" +
            std::to_string(firstX) + "," + std::to_string(firstY) + ")...(" +
            std::to_string(lastX) + "," + std::to_string(lastY) + ")";
        if (width != 1U && height != 1U)
            text += alongX ? "; thickness copies dest (Y)" : "; thickness copies dest (X)";
        return text;
    }

    EdgePortalFillResult FillEdgePortals(world::LegacyMap& map, MapEditHistory& history,
        const EdgePortalFillParams& params)
    {
        EdgePortalFillResult result;
        result.error = ValidateEdgePortalFill(map, params);
        if (!result.error.empty()) return result;

        history.Begin(map);
        for (std::uint32_t y = params.rect.top; y <= params.rect.bottom; ++y)
        {
            for (std::uint32_t x = params.rect.left; x <= params.rect.right; ++x)
            {
                auto* server = world::LegacyServerSector(map,
                    static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                if (server == nullptr)
                {
                    (void)history.Undo(map);
                    result.error = "Server sidecar data is not initialized for the selected tiles.";
                    return result;
                }
                server->portal = MakeFilledPortal(params,
                    static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                ++result.tilesWritten;
            }
        }
        map.sidecars.modified = true;
        map.modified = true;
        result.applied = true;
        return result;
    }

    bool ClearPortalsInRect(world::LegacyMap& map, MapEditHistory& history, const MapSelectionRect& rect)
    {
        if (CountPortalsInRect(map, rect) == 0U) return false;
        history.Begin(map);
        for (std::uint32_t y = rect.top; y <= rect.bottom; ++y)
        {
            for (std::uint32_t x = rect.left; x <= rect.right; ++x)
            {
                auto* server = world::LegacyServerSector(map,
                    static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                if (server != nullptr) server->portal.reset();
            }
        }
        map.sidecars.modified = true;
        map.modified = true;
        return true;
    }
}
