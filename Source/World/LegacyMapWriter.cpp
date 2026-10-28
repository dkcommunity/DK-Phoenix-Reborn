#include "World/LegacyMapWriter.h"

#include <fstream>
#include <limits>
#include <vector>

namespace phoenix::world
{
    namespace
    {
        class Writer final
        {
        public:
            void U8(const std::uint8_t value) { bytes.push_back(value); }
            void Bool(const bool value) { U8(value ? 1U : 0U); }
            void U16(const std::uint16_t value)
            {
                U8(static_cast<std::uint8_t>(value));
                U8(static_cast<std::uint8_t>(value >> 8U));
            }
            void U32(const std::uint32_t value)
            {
                U8(static_cast<std::uint8_t>(value));
                U8(static_cast<std::uint8_t>(value >> 8U));
                U8(static_cast<std::uint8_t>(value >> 16U));
                U8(static_cast<std::uint8_t>(value >> 24U));
            }
            void I32(const std::int32_t value) { U32(static_cast<std::uint32_t>(value)); }
            bool String(const std::string& value)
            {
                if (value.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) return false;
                I32(static_cast<std::int32_t>(value.size()));
                bytes.insert(bytes.end(), value.begin(), value.end());
                return true;
            }
            void PatchU32(const std::size_t offset, const std::uint32_t value)
            {
                for (std::size_t i = 0; i < 4U; ++i) bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8U));
            }

            std::vector<std::uint8_t> bytes;
        };

        bool WriteObject(Writer& writer, const LegacyMapImageObject& object)
        {
            writer.U8(object.wrapperType);
            writer.U8(object.objectType);
            writer.U32(object.objectId);
            writer.U16(object.x);
            writer.U16(object.y);
            writer.U32(object.imageObjectId);
            writer.U16(object.spriteId);
            writer.I32(object.pixelX);
            writer.I32(object.pixelY);
            writer.U16(object.viewpoint);
            writer.Bool(object.animation);
            writer.Bool(object.transparent);
            if (object.wrapperType == 5U || object.wrapperType == 6U || object.wrapperType == 7U)
            {
                writer.U16(object.frameId);
                writer.U8(object.maxFrame);
                writer.U8(object.bltType);
                writer.U8(object.direction);
                writer.U8(object.soundFrame);
                writer.U16(object.soundId);
                writer.Bool(object.loop);
                writer.U32(object.minDelay);
                writer.U32(object.maxDelay);
                writer.U8(object.startHour);
                writer.U8(object.endHour);
                if (object.wrapperType == 7U) writer.U16(object.interactionType);
            }
            else if (object.wrapperType != 3U && object.wrapperType != 4U)
            {
                return false;
            }
            if (object.positions.size() > std::numeric_limits<std::uint16_t>::max()) return false;
            writer.U16(static_cast<std::uint16_t>(object.positions.size()));
            for (const auto& [x, y] : object.positions)
            {
                writer.U16(x);
                writer.U16(y);
            }
            return true;
        }

        bool WriteFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
        {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) return false;
            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return file.good();
        }
    }

    LegacyMapSerializationResult LegacyMapWriter::Serialize(const LegacyMap& map) const
    {
        LegacyMapSerializationResult result;

        if (!map.modified && !map.originalBytes.empty())
        {
            result.succeeded = true;
            result.originalBytesPreserved = true;
            result.bytes = map.originalBytes;
            return result;
        }

        const bool map2000 = map.version == "=MAP_2000_05_10=";
        const bool map2024 = map.version == "=MAP_2024_04_30=";
        if ((!map2000 && !map2024) || map.width == 0U || map.height == 0U ||
            map.sectors.size() != static_cast<std::size_t>(map.width) * map.height)
        {
            result.error = "Map model is not a supported Legacy map document.";
            return result;
        }

        Writer writer;
        if (!writer.String(map.version)) { result.error = "Map version string is too long."; return result; }
        writer.U16(map.zoneId);
        writer.U16(map.zoneGroupId);
        if (!writer.String(map.name)) { result.error = "Map name is too long."; return result; }
        writer.U8(map.zoneType);
        writer.U8(map.zoneLevel);
        if (!writer.String(map.description)) { result.error = "Map description is too long."; return result; }
        const auto pointerOffset = writer.bytes.size();
        writer.U32(0U);
        writer.U32(0U);
        writer.U16(map.width);
        writer.U16(map.height);
        for (const auto& sector : map.sectors)
        {
            if (map2024)
            {
                writer.U32(sector.spriteId);
            }
            else if (sector.spriteId > std::numeric_limits<std::uint16_t>::max())
            {
                result.error = "MAP_2000_05_10 tile sprite ID exceeds the Legacy uint16 range.";
                return result;
            }
            else
            {
                writer.U16(static_cast<std::uint16_t>(sector.spriteId));
            }
            writer.U8(sector.property);
            writer.U8(sector.light);
        }
        if (map.imageObjects.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        {
            result.error = "Map has too many image objects.";
            return result;
        }
        writer.I32(static_cast<std::int32_t>(map.imageObjects.size()));
        for (const auto& object : map.imageObjects)
        {
            if (!WriteObject(writer, object))
            {
                result.error = "Map contains an unsupported or oversized image object record.";
                return result;
            }
        }

        if (writer.bytes.size() > std::numeric_limits<std::uint32_t>::max())
        {
            result.error = "Map exceeds the Legacy 32-bit pointer limit.";
            return result;
        }
        const auto tilePointer = static_cast<std::uint32_t>(writer.bytes.size());
        if (map.tileSpriteFilePositions.size() > std::numeric_limits<std::uint16_t>::max())
        {
            result.error = "Tile resource list exceeds the Legacy limit.";
            return result;
        }
        writer.U16(static_cast<std::uint16_t>(map.tileSpriteFilePositions.size()));
        for (const auto& entry : map.tileSpriteFilePositions)
        {
            if (map2024)
            {
                writer.U32(entry.spriteId);
            }
            else if (entry.spriteId > std::numeric_limits<std::uint16_t>::max())
            {
                result.error = "MAP_2000_05_10 tile resource ID exceeds the Legacy uint16 range.";
                return result;
            }
            else
            {
                writer.U16(static_cast<std::uint16_t>(entry.spriteId));
            }
            writer.I32(entry.filePosition);
        }

        const auto imagePointer = static_cast<std::uint32_t>(writer.bytes.size());
        if (map.imageObjectSpriteFilePositions.size() > std::numeric_limits<std::uint16_t>::max())
        {
            result.error = "Image-object resource list exceeds the Legacy limit.";
            return result;
        }
        writer.U16(static_cast<std::uint16_t>(map.imageObjectSpriteFilePositions.size()));
        for (const auto& entry : map.imageObjectSpriteFilePositions)
        {
            if (entry.spriteId > std::numeric_limits<std::uint16_t>::max())
            {
                result.error = "Image-object resource ID exceeds the Legacy uint16 range.";
                return result;
            }
            writer.U16(static_cast<std::uint16_t>(entry.spriteId));
            writer.I32(entry.filePosition);
        }
        writer.PatchU32(pointerOffset, tilePointer);
        writer.PatchU32(pointerOffset + 4U, imagePointer);

        result.succeeded = true;
        result.bytes = std::move(writer.bytes);
        return result;
    }

    LegacyMapWriteResult LegacyMapWriter::Save(const LegacyMap& map, const std::filesystem::path& path) const
    {
        LegacyMapWriteResult result;
        if (path.empty())
        {
            result.error = "Map output path is empty.";
            return result;
        }

        const auto serialized = Serialize(map);
        if (!serialized.succeeded)
        {
            result.error = serialized.error;
            return result;
        }

        std::error_code ec;
        if (std::filesystem::exists(path, ec))
        {
            result.backupPath = path;
            result.backupPath += L".bak";
            std::filesystem::copy_file(path, result.backupPath, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec)
            {
                result.error = "Automatic backup failed: " + ec.message();
                return result;
            }
            result.backupCreated = true;
        }

        result.succeeded = WriteFile(path, serialized.bytes);
        result.originalBytesPreserved = result.succeeded && serialized.originalBytesPreserved;
        if (!result.succeeded) result.error = "Serialized map could not be written.";
        return result;
    }
}
