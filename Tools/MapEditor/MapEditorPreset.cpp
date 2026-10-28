#include "MapEditorPreset.h"

#include "MapEditorDocument.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <type_traits>

namespace phoenix::tools::map_editor
{
    namespace
    {
        constexpr std::array<std::uint8_t, 8> kMagic{ 'P', 'H', 'X', 'P', 'R', 'S', 'T', '1' };
        constexpr std::uint32_t kMaximumNameBytes = 240U;
        constexpr std::uint32_t kMaximumTiles = 2U * 1024U * 1024U;
        constexpr std::uint32_t kMaximumObjects = 256U * 1024U;
        constexpr std::uint32_t kMaximumPositions = 4U * 1024U * 1024U;
        constexpr std::uint64_t kMaximumPayloadBytes = 64ULL * 1024ULL * 1024ULL;
        constexpr std::int32_t kTilePixelWidth = 48;
        constexpr std::int32_t kTilePixelHeight = 24;

        class ByteWriter final
        {
        public:
            template <typename T>
            void Write(const T value)
            {
                static_assert(std::is_integral_v<T>);
                using U = std::make_unsigned_t<T>;
                const auto encoded = static_cast<U>(value);
                for (std::size_t index = 0; index < sizeof(T); ++index)
                    bytes.push_back(static_cast<std::uint8_t>((encoded >> (index * 8U)) & 0xFFU));
            }

            void WriteString(const std::string& value)
            {
                Write<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
                bytes.insert(bytes.end(), value.begin(), value.end());
            }

            std::vector<std::uint8_t> bytes;
        };

        class ByteReader final
        {
        public:
            explicit ByteReader(const std::vector<std::uint8_t>& source) : source_(source) {}

            template <typename T>
            bool Read(T& value)
            {
                static_assert(std::is_integral_v<T>);
                if (source_.size() - offset_ < sizeof(T)) return Fail("Unexpected end of preset data.");
                using U = std::make_unsigned_t<T>;
                U decoded = 0;
                for (std::size_t index = 0; index < sizeof(T); ++index)
                    decoded |= static_cast<U>(source_[offset_ + index]) << (index * 8U);
                value = static_cast<T>(decoded);
                offset_ += sizeof(T);
                return true;
            }

            bool ReadString(std::string& value, const std::uint32_t maximum)
            {
                std::uint32_t size = 0;
                if (!Read(size)) return false;
                if (size > maximum) return Fail("Preset string exceeds its format limit.");
                if (source_.size() - offset_ < size) return Fail("Preset string is truncated.");
                value.assign(reinterpret_cast<const char*>(source_.data() + offset_), size);
                offset_ += size;
                return true;
            }

            [[nodiscard]] bool AtEnd() const noexcept { return offset_ == source_.size(); }
            [[nodiscard]] const std::string& Error() const noexcept { return error_; }

        private:
            bool Fail(const char* message)
            {
                if (error_.empty()) error_ = message;
                return false;
            }

            const std::vector<std::uint8_t>& source_;
            std::size_t offset_{ 0 };
            std::string error_;
        };

        std::uint32_t Crc32(const std::vector<std::uint8_t>& bytes)
        {
            std::uint32_t crc = 0xFFFFFFFFU;
            for (const auto byte : bytes)
            {
                crc ^= byte;
                for (unsigned bit = 0; bit < 8U; ++bit)
                    crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
            }
            return ~crc;
        }

        MapPresetResource FindResource(
            const std::vector<world::LegacyMapSpriteFilePosition>& resources, const std::uint32_t spriteId)
        {
            const auto found = std::find_if(resources.begin(), resources.end(), [spriteId](const auto& resource) {
                return resource.spriteId == spriteId;
            });
            if (found == resources.end()) return {};
            return { true, found->spriteId, found->filePosition };
        }

        bool ObjectTouchesSelection(const world::LegacyMapImageObject& object, const MapPresetRect& selection)
        {
            for (const auto& position : object.positions)
            {
                if (position.first >= selection.left && position.first <= selection.right &&
                    position.second >= selection.top && position.second <= selection.bottom) return true;
            }
            if (!object.positions.empty()) return false;
            const auto x = object.pixelX / kTilePixelWidth;
            const auto y = object.pixelY / kTilePixelHeight;
            return x >= selection.left && x <= selection.right && y >= selection.top && y <= selection.bottom;
        }

        void WriteResource(ByteWriter& writer, const MapPresetResource& resource)
        {
            writer.Write<std::uint8_t>(resource.present ? 1U : 0U);
            writer.Write<std::uint32_t>(resource.spriteId);
            writer.Write<std::int32_t>(resource.filePosition);
        }

        bool ReadResource(ByteReader& reader, MapPresetResource& resource)
        {
            std::uint8_t present = 0;
            if (!reader.Read(present) || present > 1U || !reader.Read(resource.spriteId) ||
                !reader.Read(resource.filePosition)) return false;
            resource.present = present != 0U;
            return true;
        }

        void WriteObject(ByteWriter& writer, const MapPresetObject& entry)
        {
            const auto& object = entry.object;
            writer.Write<std::uint8_t>(object.wrapperType);
            writer.Write<std::uint8_t>(object.objectType);
            writer.Write<std::uint32_t>(object.objectId);
            writer.Write<std::uint16_t>(object.x);
            writer.Write<std::uint16_t>(object.y);
            writer.Write<std::uint32_t>(object.imageObjectId);
            writer.Write<std::uint16_t>(object.spriteId);
            writer.Write<std::int32_t>(entry.relativePixelX);
            writer.Write<std::int32_t>(entry.relativePixelY);
            writer.Write<std::int32_t>(entry.relativeViewpoint);
            writer.Write<std::uint8_t>(entry.preBackground ? 1U : 0U);
            writer.Write<std::uint8_t>(object.animation ? 1U : 0U);
            writer.Write<std::uint8_t>(object.transparent ? 1U : 0U);
            writer.Write<std::uint16_t>(object.frameId);
            writer.Write<std::uint8_t>(object.maxFrame);
            writer.Write<std::uint8_t>(object.bltType);
            writer.Write<std::uint8_t>(object.direction);
            writer.Write<std::uint8_t>(object.soundFrame);
            writer.Write<std::uint16_t>(object.soundId);
            writer.Write<std::uint8_t>(object.loop ? 1U : 0U);
            writer.Write<std::uint32_t>(object.minDelay);
            writer.Write<std::uint32_t>(object.maxDelay);
            writer.Write<std::uint8_t>(object.startHour);
            writer.Write<std::uint8_t>(object.endHour);
            writer.Write<std::uint16_t>(object.interactionType);
            writer.Write<std::uint32_t>(static_cast<std::uint32_t>(entry.relativePositions.size()));
            for (const auto& position : entry.relativePositions)
            {
                writer.Write<std::int32_t>(position.first);
                writer.Write<std::int32_t>(position.second);
            }
            WriteResource(writer, entry.resource);
        }

        bool ReadObject(ByteReader& reader, MapPresetObject& entry, std::uint64_t& totalPositions)
        {
            auto& object = entry.object;
            std::uint8_t preBackground = 0, animation = 0, transparent = 0, loop = 0;
            std::uint32_t positionCount = 0;
            if (!reader.Read(object.wrapperType) || !reader.Read(object.objectType) || !reader.Read(object.objectId) ||
                !reader.Read(object.x) || !reader.Read(object.y) || !reader.Read(object.imageObjectId) ||
                !reader.Read(object.spriteId) || !reader.Read(entry.relativePixelX) ||
                !reader.Read(entry.relativePixelY) || !reader.Read(entry.relativeViewpoint) ||
                !reader.Read(preBackground) || !reader.Read(animation) || !reader.Read(transparent) ||
                !reader.Read(object.frameId) || !reader.Read(object.maxFrame) || !reader.Read(object.bltType) ||
                !reader.Read(object.direction) || !reader.Read(object.soundFrame) || !reader.Read(object.soundId) ||
                !reader.Read(loop) || !reader.Read(object.minDelay) || !reader.Read(object.maxDelay) ||
                !reader.Read(object.startHour) || !reader.Read(object.endHour) || !reader.Read(object.interactionType) ||
                !reader.Read(positionCount)) return false;
            if (preBackground > 1U || animation > 1U || transparent > 1U || loop > 1U) return false;
            totalPositions += positionCount;
            if (totalPositions > kMaximumPositions) return false;
            entry.preBackground = preBackground != 0U;
            object.animation = animation != 0U;
            object.transparent = transparent != 0U;
            object.loop = loop != 0U;
            entry.relativePositions.resize(positionCount);
            for (auto& position : entry.relativePositions)
                if (!reader.Read(position.first) || !reader.Read(position.second)) return false;
            return ReadResource(reader, entry.resource);
        }

        MapPresetResult ValidatePreset(const MapPreset& preset)
        {
            if (preset.formatVersion != kMapPresetFormatVersion) return { false, "Unsupported preset version." };
            const auto name = ValidateMapPresetName(preset.name);
            if (!name.succeeded) return name;
            if (preset.width == 0U || preset.height == 0U) return { false, "Preset bounds are empty." };
            const auto expectedTiles = static_cast<std::uint64_t>(preset.width) * preset.height;
            if (expectedTiles > kMaximumTiles) return { false, "Preset tile count exceeds its safety limit." };
            if (expectedTiles != preset.tiles.size()) return { false, "Preset tile count does not match its bounds." };
            if (preset.tiles.size() > kMaximumTiles || preset.objects.size() > kMaximumObjects)
                return { false, "Preset exceeds the supported safety limits." };
            for (std::size_t tileIndex = 0; tileIndex < preset.tiles.size(); ++tileIndex)
            {
                const auto& tile = preset.tiles[tileIndex];
                if (tile.relativeX < 0 || tile.relativeY < 0 || tile.relativeX >= preset.width || tile.relativeY >= preset.height)
                    return { false, "Preset contains a tile outside its declared bounds." };
                if (tile.relativeX != static_cast<std::int32_t>(tileIndex % preset.width) ||
                    tile.relativeY != static_cast<std::int32_t>(tileIndex / preset.width))
                    return { false, "Preset tile records are not in canonical row-major order." };
            }
            std::uint64_t positions = 0U;
            for (const auto& object : preset.objects)
            {
                positions += object.relativePositions.size();
                if (positions > kMaximumPositions) return { false, "Preset object positions exceed the safety limit." };
            }
            return { true, {} };
        }

        void AddResourceIfMissing(std::vector<world::LegacyMapSpriteFilePosition>& resources,
            const MapPresetResource& resource)
        {
            if (!resource.present) return;
            const auto found = std::find_if(resources.begin(), resources.end(), [&](const auto& candidate) {
                return candidate.spriteId == resource.spriteId;
            });
            if (found == resources.end()) resources.push_back({ resource.spriteId, resource.filePosition });
        }
    }

    MapPresetResult ValidateMapPresetName(const std::string& name)
    {
        if (name.empty()) return { false, "Preset name is required." };
        if (name.size() > kMaximumNameBytes) return { false, "Preset name is too long." };
        if (name.back() == '.' || name.back() == ' ') return { false, "Preset name cannot end with a dot or space." };
        constexpr const char* invalid = "<>:\"/\\|?*";
        for (const auto character : name)
        {
            if (static_cast<unsigned char>(character) < 32U || std::strchr(invalid, character) != nullptr)
                return { false, "Preset name contains a character that is invalid in a filename." };
        }
        return { true, {} };
    }

    MapPresetCaptureResult CaptureMapPreset(
        const world::LegacyMap& map, const MapPresetRect& selection, std::string name)
    {
        if (!selection.IsValid() || selection.right >= map.width || selection.bottom >= map.height)
            return { std::nullopt, "Preset selection is outside the map bounds." };
        if (map.sectors.size() != static_cast<std::size_t>(map.width) * map.height)
            return { std::nullopt, "Map sector data is incomplete; preset capture was not performed." };
        const auto nameResult = ValidateMapPresetName(name);
        if (!nameResult.succeeded) return { std::nullopt, nameResult.error };

        MapPreset preset;
        preset.name = std::move(name);
        preset.originX = selection.left;
        preset.originY = selection.top;
        preset.width = static_cast<std::uint16_t>(selection.right - selection.left + 1U);
        preset.height = static_cast<std::uint16_t>(selection.bottom - selection.top + 1U);
        preset.tiles.reserve(static_cast<std::size_t>(preset.width) * preset.height);
        for (std::uint16_t y = selection.top; y <= selection.bottom; ++y)
        {
            for (std::uint16_t x = selection.left; x <= selection.right; ++x)
            {
                const auto index = static_cast<std::size_t>(y) * map.width + x;
                const auto& sector = map.sectors[index];
                preset.tiles.push_back({ static_cast<std::int32_t>(x - selection.left),
                    static_cast<std::int32_t>(y - selection.top), sector,
                    FindResource(map.tileSpriteFilePositions, sector.spriteId) });
            }
        }

        for (const auto& object : map.imageObjects)
        {
            if (!ObjectTouchesSelection(object, selection)) continue;
            MapPresetObject entry;
            entry.object = object;
            entry.object.positions.clear();
            entry.relativePixelX = object.pixelX - static_cast<std::int32_t>(selection.left) * kTilePixelWidth;
            entry.relativePixelY = object.pixelY - static_cast<std::int32_t>(selection.top) * kTilePixelHeight;
            entry.preBackground = object.viewpoint == 0U;
            entry.relativeViewpoint = entry.preBackground ? 0 :
                static_cast<std::int32_t>(object.viewpoint) - selection.top;
            entry.relativePositions.reserve(object.positions.size());
            for (const auto& position : object.positions)
                entry.relativePositions.emplace_back(static_cast<std::int32_t>(position.first) - selection.left,
                    static_cast<std::int32_t>(position.second) - selection.top);
            entry.resource = FindResource(map.imageObjectSpriteFilePositions, object.spriteId);
            preset.objects.push_back(std::move(entry));
        }
        return { std::move(preset), {} };
    }

    MapPresetResult ValidateMapPresetPlacement(
        const world::LegacyMap& map, const MapPreset& preset, const std::uint16_t targetX, const std::uint16_t targetY)
    {
        const auto valid = ValidatePreset(preset);
        if (!valid.succeeded) return valid;
        if (map.sectors.size() != static_cast<std::size_t>(map.width) * map.height)
            return { false, "Map sector data is incomplete; preset placement was not performed." };
        if (static_cast<std::uint64_t>(targetX) + preset.width > map.width ||
            static_cast<std::uint64_t>(targetY) + preset.height > map.height)
            return { false, "Preset placement extends beyond the map bounds." };
        for (const auto& entry : preset.objects)
        {
            if (!entry.preBackground)
            {
                const auto viewpoint = static_cast<std::int64_t>(targetY) + entry.relativeViewpoint;
                if (viewpoint < 0 || viewpoint > std::numeric_limits<std::uint16_t>::max())
                    return { false, "Preset placement produces an invalid object viewpoint." };
            }
            for (const auto& position : entry.relativePositions)
            {
                const auto x = static_cast<std::int64_t>(targetX) + position.first;
                const auto y = static_cast<std::int64_t>(targetY) + position.second;
                if (x < 0 || y < 0 || x >= map.width || y >= map.height)
                    return { false, "Preset placement produces an object position outside the map." };
            }
        }
        std::uint32_t maximumId = 0;
        for (const auto& object : map.imageObjects) maximumId = std::max(maximumId, object.objectId);
        if (preset.objects.size() > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) - maximumId)
            return { false, "Map object identifiers are exhausted." };
        return { true, {} };
    }

    MapPresetPlacementResult PlaceMapPreset(
        world::LegacyMap& map, const MapPreset& preset, const std::uint16_t targetX, const std::uint16_t targetY)
    {
        const auto valid = ValidateMapPresetPlacement(map, preset, targetX, targetY);
        if (!valid.succeeded) return { false, {}, valid.error };
        for (const auto& tile : preset.tiles)
        {
            const auto x = static_cast<std::size_t>(targetX + tile.relativeX);
            const auto y = static_cast<std::size_t>(targetY + tile.relativeY);
            map.sectors[y * map.width + x] = tile.sector;
            AddResourceIfMissing(map.tileSpriteFilePositions, tile.resource);
        }

        std::uint32_t nextId = 0;
        for (const auto& object : map.imageObjects) nextId = std::max(nextId, object.objectId);
        MapPresetPlacementResult result{ true, {}, {} };
        result.objectIds.reserve(preset.objects.size());
        for (const auto& entry : preset.objects)
        {
            auto object = entry.object;
            object.objectId = ++nextId;
            object.imageObjectId = object.objectId;
            object.pixelX = static_cast<std::int32_t>(targetX) * kTilePixelWidth + entry.relativePixelX;
            object.pixelY = static_cast<std::int32_t>(targetY) * kTilePixelHeight + entry.relativePixelY;
            object.viewpoint = entry.preBackground ? 0U :
                static_cast<std::uint16_t>(static_cast<std::int32_t>(targetY) + entry.relativeViewpoint);
            object.positions.clear();
            object.positions.reserve(entry.relativePositions.size());
            for (const auto& position : entry.relativePositions)
                object.positions.emplace_back(static_cast<std::uint16_t>(targetX + position.first),
                    static_cast<std::uint16_t>(targetY + position.second));
            result.objectIds.push_back(object.objectId);
            map.imageObjects.push_back(std::move(object));
            AddResourceIfMissing(map.imageObjectSpriteFilePositions, entry.resource);
        }
        RebuildObjectIndex(map);
        map.modified = true;
        return result;
    }

    MapPresetResult SaveMapPreset(const MapPreset& preset, const std::filesystem::path& path)
    {
        const auto valid = ValidatePreset(preset);
        if (!valid.succeeded) return valid;
        ByteWriter payload;
        payload.WriteString(preset.name);
        payload.Write<std::int32_t>(preset.originX);
        payload.Write<std::int32_t>(preset.originY);
        payload.Write<std::uint16_t>(preset.width);
        payload.Write<std::uint16_t>(preset.height);
        payload.Write<std::uint32_t>(static_cast<std::uint32_t>(preset.tiles.size()));
        payload.Write<std::uint32_t>(static_cast<std::uint32_t>(preset.objects.size()));
        for (const auto& tile : preset.tiles)
        {
            payload.Write<std::int32_t>(tile.relativeX);
            payload.Write<std::int32_t>(tile.relativeY);
            payload.Write<std::uint32_t>(tile.sector.spriteId);
            payload.Write<std::uint8_t>(tile.sector.property);
            payload.Write<std::uint8_t>(tile.sector.light);
            WriteResource(payload, tile.resource);
        }
        for (const auto& object : preset.objects) WriteObject(payload, object);
        if (payload.bytes.size() > kMaximumPayloadBytes) return { false, "Preset payload is too large." };

        ByteWriter file;
        file.bytes.insert(file.bytes.end(), kMagic.begin(), kMagic.end());
        file.Write<std::uint16_t>(kMapPresetFormatVersion);
        file.Write<std::uint16_t>(0U);
        file.Write<std::uint64_t>(payload.bytes.size());
        file.Write<std::uint32_t>(Crc32(payload.bytes));
        file.bytes.insert(file.bytes.end(), payload.bytes.begin(), payload.bytes.end());

        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) return { false, "Could not create the Presets directory: " + error.message() };
        const auto temporary = std::filesystem::path(path.wstring() + L".tmp");
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) return { false, "Could not open the temporary preset file." };
            output.write(reinterpret_cast<const char*>(file.bytes.data()), static_cast<std::streamsize>(file.bytes.size()));
            output.flush();
            if (!output) return { false, "Could not write the complete preset file." };
        }
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            std::filesystem::remove(temporary, error);
            return { false, "Could not commit the preset file." };
        }
        return { true, {} };
    }

    MapPresetLoadResult LoadMapPreset(const std::filesystem::path& path)
    {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        constexpr std::size_t headerSize = 8U + 2U + 2U + 8U + 4U;
        if (error || size < headerSize || size > kMaximumPayloadBytes + headerSize)
            return { std::nullopt, "Preset file size is invalid." };
        std::ifstream input(path, std::ios::binary);
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) return { std::nullopt, "Preset file could not be read completely." };
        if (!std::equal(kMagic.begin(), kMagic.end(), bytes.begin()))
            return { std::nullopt, "Preset magic is invalid." };

        std::vector<std::uint8_t> headerBytes(bytes.begin() + 8, bytes.begin() + headerSize);
        ByteReader header(headerBytes);
        std::uint16_t version = 0, reserved = 0;
        std::uint64_t payloadSize = 0;
        std::uint32_t expectedCrc = 0;
        if (!header.Read(version) || !header.Read(reserved) || !header.Read(payloadSize) || !header.Read(expectedCrc) ||
            version != kMapPresetFormatVersion || reserved != 0U || payloadSize != bytes.size() - headerSize)
            return { std::nullopt, "Preset header is invalid or unsupported." };
        std::vector<std::uint8_t> payload(bytes.begin() + headerSize, bytes.end());
        if (Crc32(payload) != expectedCrc) return { std::nullopt, "Preset checksum validation failed." };

        ByteReader reader(payload);
        MapPreset preset;
        preset.formatVersion = version;
        std::uint32_t tileCount = 0, objectCount = 0;
        if (!reader.ReadString(preset.name, kMaximumNameBytes) || !reader.Read(preset.originX) ||
            !reader.Read(preset.originY) || !reader.Read(preset.width) || !reader.Read(preset.height) ||
            !reader.Read(tileCount) || !reader.Read(objectCount))
            return { std::nullopt, reader.Error() };
        if (tileCount > kMaximumTiles || objectCount > kMaximumObjects)
            return { std::nullopt, "Preset record count exceeds its safety limit." };
        preset.tiles.resize(tileCount);
        for (auto& tile : preset.tiles)
        {
            if (!reader.Read(tile.relativeX) || !reader.Read(tile.relativeY) || !reader.Read(tile.sector.spriteId) ||
                !reader.Read(tile.sector.property) || !reader.Read(tile.sector.light) || !ReadResource(reader, tile.resource))
                return { std::nullopt, reader.Error().empty() ? "Preset tile record is invalid." : reader.Error() };
        }
        preset.objects.resize(objectCount);
        std::uint64_t totalPositions = 0;
        for (auto& object : preset.objects)
        {
            if (!ReadObject(reader, object, totalPositions))
                return { std::nullopt, reader.Error().empty() ? "Preset object record is invalid." : reader.Error() };
        }
        if (!reader.AtEnd()) return { std::nullopt, "Preset contains unsupported trailing data." };
        const auto valid = ValidatePreset(preset);
        if (!valid.succeeded) return { std::nullopt, valid.error };
        return { std::move(preset), {} };
    }
}
