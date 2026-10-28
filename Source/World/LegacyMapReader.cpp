#include "World/LegacyMapReader.h"

#include <fstream>
#include <algorithm>
#include <iterator>
#include <limits>
#include <sstream>
#include <utility>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
#include <windows.h>
#endif

namespace phoenix::world
{
    namespace
    {
        constexpr const char* kSupportedMapVersion = "=MAP_2000_05_10=";
        constexpr const char* kLayoutCompatibleMapVersion = "=MAP_2024_04_30=";
        constexpr std::uint8_t kTypeImageObject = 3;
        constexpr std::uint8_t kTypeShadowObject = 4;
        constexpr std::uint8_t kTypeAnimationObject = 5;
        constexpr std::uint8_t kTypeShadowAnimationObject = 6;
        constexpr std::uint8_t kTypeInteractionObject = 7;

        class LegacyBinaryReader final
        {
        public:
            explicit LegacyBinaryReader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes))
            {
            }

            [[nodiscard]] std::size_t Offset() const noexcept
            {
                return offset_;
            }

            [[nodiscard]] std::size_t Size() const noexcept
            {
                return bytes_.size();
            }

            [[nodiscard]] std::size_t Remaining() const noexcept
            {
                return offset_ <= bytes_.size() ? bytes_.size() - offset_ : 0U;
            }

            [[nodiscard]] bool Seek(const std::size_t offset) noexcept
            {
                if (offset > bytes_.size())
                {
                    return false;
                }
                offset_ = offset;
                return true;
            }

            [[nodiscard]] bool U8(std::uint8_t& value) noexcept
            {
                if (offset_ + 1U > bytes_.size())
                {
                    return false;
                }
                value = bytes_[offset_++];
                return true;
            }

            [[nodiscard]] bool Bool(bool& value) noexcept
            {
                std::uint8_t raw = 0;
                if (!U8(raw))
                {
                    return false;
                }
                value = raw != 0;
                return true;
            }

            [[nodiscard]] bool U16(std::uint16_t& value) noexcept
            {
                if (offset_ + 2U > bytes_.size())
                {
                    return false;
                }
                value = static_cast<std::uint16_t>(bytes_[offset_]) |
                    static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[offset_ + 1U]) << 8U);
                offset_ += 2U;
                return true;
            }

            [[nodiscard]] bool U32(std::uint32_t& value) noexcept
            {
                if (offset_ + 4U > bytes_.size())
                {
                    return false;
                }
                value = static_cast<std::uint32_t>(bytes_[offset_]) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 1U]) << 8U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 2U]) << 16U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 3U]) << 24U);
                offset_ += 4U;
                return true;
            }

            [[nodiscard]] bool I32(std::int32_t& value) noexcept
            {
                std::uint32_t raw = 0;
                if (!U32(raw))
                {
                    return false;
                }
                value = static_cast<std::int32_t>(raw);
                return true;
            }

            [[nodiscard]] bool String(std::string& value) noexcept
            {
                std::int32_t length = 0;
                if (!I32(length) || length < 0 || length > 65535 || offset_ + static_cast<std::size_t>(length) > bytes_.size())
                {
                    return false;
                }
                value.assign(
                    reinterpret_cast<const char*>(bytes_.data() + offset_),
                    reinterpret_cast<const char*>(bytes_.data() + offset_ + static_cast<std::size_t>(length)));
                offset_ += static_cast<std::size_t>(length);
                return true;
            }

            [[nodiscard]] bool Skip(const std::size_t count) noexcept
            {
                if (offset_ + count > bytes_.size())
                {
                    return false;
                }
                offset_ += count;
                return true;
            }

        private:
            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };

        std::vector<std::uint8_t> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            const std::vector<char> raw{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };
            std::vector<std::uint8_t> bytes;
            bytes.reserve(raw.size());
            for (const auto byte : raw)
            {
                bytes.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(byte)));
            }
            return bytes;
        }

        [[nodiscard]] bool ReadZoneHeader(
            LegacyBinaryReader& reader, LegacyMap& map, std::string& failedField, std::size_t& expectedBytes)
        {
            if (!reader.String(map.version)) { failedField = "version string"; expectedBytes = 4U; return false; }
            if (!reader.U16(map.zoneId)) { failedField = "zoneId"; expectedBytes = 2U; return false; }
            if (!reader.U16(map.zoneGroupId)) { failedField = "zoneGroupId"; expectedBytes = 2U; return false; }
            if (!reader.String(map.name)) { failedField = "map name string"; expectedBytes = 4U; return false; }
            if (!reader.U8(map.zoneType)) { failedField = "zoneType"; expectedBytes = 1U; return false; }
            if (!reader.U8(map.zoneLevel)) { failedField = "zoneLevel"; expectedBytes = 1U; return false; }
            if (!reader.String(map.description)) { failedField = "description string"; expectedBytes = 4U; return false; }
            return true;
        }

        [[nodiscard]] bool ReadPositionList(LegacyBinaryReader& reader, LegacyMapImageObject& object,
            std::string& failedField, std::size_t& expectedBytes, std::uint64_t& parsedCount)
        {
            std::uint16_t count = 0;
            if (!reader.U16(count))
            {
                failedField = "position count";
                expectedBytes = 2U;
                return false;
            }
            parsedCount = count;
            if (static_cast<std::size_t>(count) > reader.Remaining() / 4U)
            {
                failedField = "position records";
                expectedBytes = static_cast<std::size_t>(count) * 4U;
                return false;
            }
            object.positions.reserve(count);
            for (std::uint16_t i = 0; i < count; ++i)
            {
                std::uint16_t x = 0;
                std::uint16_t y = 0;
                if (!reader.U16(x) || !reader.U16(y))
                {
                    failedField = "position x/y";
                    expectedBytes = 4U;
                    return false;
                }
                object.positions.emplace_back(x, y);
            }
            return true;
        }

        [[nodiscard]] bool ReadAnimationTrailer(LegacyBinaryReader& reader, LegacyMapImageObject& object,
            std::string& failedField, std::size_t& expectedBytes)
        {
            if (!reader.U16(object.frameId)) { failedField = "frameId"; expectedBytes = 2U; return false; }
            if (!reader.U8(object.maxFrame)) { failedField = "maxFrame"; expectedBytes = 1U; return false; }
            if (!reader.U8(object.bltType)) { failedField = "bltType"; expectedBytes = 1U; return false; }
            if (!reader.U8(object.direction)) { failedField = "direction"; expectedBytes = 1U; return false; }
            if (!reader.U8(object.soundFrame)) { failedField = "soundFrame"; expectedBytes = 1U; return false; }
            if (!reader.U16(object.soundId)) { failedField = "soundId"; expectedBytes = 2U; return false; }
            if (!reader.Bool(object.loop)) { failedField = "loop"; expectedBytes = 1U; return false; }
            if (!reader.U32(object.minDelay)) { failedField = "minDelay"; expectedBytes = 4U; return false; }
            if (!reader.U32(object.maxDelay)) { failedField = "maxDelay"; expectedBytes = 4U; return false; }
            if (!reader.U8(object.startHour)) { failedField = "startHour"; expectedBytes = 1U; return false; }
            if (!reader.U8(object.endHour)) { failedField = "endHour"; expectedBytes = 1U; return false; }
            return true;
        }

        [[nodiscard]] bool ReadImageObject(LegacyBinaryReader& reader, LegacyMapImageObject& object,
            std::string& failedField, std::size_t& expectedBytes)
        {
            if (!reader.U8(object.wrapperType)) { failedField = "wrapperType"; expectedBytes = 1U; return false; }
            if (!reader.U8(object.objectType)) { failedField = "objectType"; expectedBytes = 1U; return false; }
            if (!reader.U32(object.objectId)) { failedField = "objectId"; expectedBytes = 4U; return false; }
            if (!reader.U16(object.x)) { failedField = "wallDirection"; expectedBytes = 2U; return false; }
            if (!reader.U16(object.y)) { failedField = "wallValue"; expectedBytes = 2U; return false; }
            if (!reader.U32(object.imageObjectId)) { failedField = "imageObjectId"; expectedBytes = 4U; return false; }
            if (!reader.U16(object.spriteId)) { failedField = "spriteId"; expectedBytes = 2U; return false; }
            if (!reader.I32(object.pixelX)) { failedField = "pixelX"; expectedBytes = 4U; return false; }
            if (!reader.I32(object.pixelY)) { failedField = "pixelY"; expectedBytes = 4U; return false; }
            if (!reader.U16(object.viewpoint)) { failedField = "viewpoint"; expectedBytes = 2U; return false; }
            if (!reader.Bool(object.animation)) { failedField = "animation"; expectedBytes = 1U; return false; }
            if (!reader.Bool(object.transparent)) { failedField = "transparent"; expectedBytes = 1U; return false; }

            switch (object.wrapperType)
            {
            case kTypeImageObject:
            case kTypeShadowObject:
                return true;
            case kTypeAnimationObject:
            case kTypeShadowAnimationObject:
                return ReadAnimationTrailer(reader, object, failedField, expectedBytes);
            case kTypeInteractionObject:
                if (!ReadAnimationTrailer(reader, object, failedField, expectedBytes)) return false;
                if (!reader.U16(object.interactionType)) { failedField = "interactionType"; expectedBytes = 2U; return false; }
                return true;
            default:
                failedField = "wrapperType unsupported value " + std::to_string(object.wrapperType);
                expectedBytes = 1U;
                return false;
            }
        }

        struct SpriteArrayReadResult
        {
            LegacyMapResourceArrayStatus status{ LegacyMapResourceArrayStatus::Absent };
            std::optional<LegacyMapParseDiagnostic> diagnostic;
        };

        [[nodiscard]] SpriteArrayReadResult ReadSpriteFilePositionArray(
            LegacyBinaryReader& reader,
            const std::int32_t filePointer,
            const std::size_t structuralEnd,
            const std::optional<std::int32_t> followingPointer,
            const std::string& section,
            const std::size_t spriteIdBytes,
            std::vector<LegacyMapSpriteFilePosition>& entries)
        {
            SpriteArrayReadResult result;
            if (filePointer == 0) return result;
            if (filePointer < 0 || static_cast<std::size_t>(filePointer) >= reader.Size() ||
                static_cast<std::size_t>(filePointer) < structuralEnd)
            {
                result.status = LegacyMapResourceArrayStatus::StalePointer;
                const auto diagnosticOffset = filePointer >= 0 ? static_cast<std::size_t>(filePointer) : structuralEnd;
                result.diagnostic = LegacyMapParseDiagnostic{ section, "absolute file pointer", diagnosticOffset, 2U,
                    diagnosticOffset < reader.Size() ? reader.Size() - diagnosticOffset : 0U, 0U, false };
                return result;
            }
            (void)reader.Seek(static_cast<std::size_t>(filePointer));
            std::uint16_t count = 0;
            if (!reader.U16(count))
            {
                result.status = LegacyMapResourceArrayStatus::MalformedAdvisory;
                result.diagnostic = LegacyMapParseDiagnostic{ section, "array count", static_cast<std::size_t>(filePointer),
                    2U, reader.Remaining(), 0U, false };
                return result;
            }
            if (count == 0U) { result.status = LegacyMapResourceArrayStatus::Empty; return result; }
            const auto elementBytes = spriteIdBytes + 4U;
            const auto expected = 2U + static_cast<std::size_t>(count) * elementBytes;
            auto available = reader.Size() - static_cast<std::size_t>(filePointer);
            if (followingPointer && *followingPointer > filePointer && static_cast<std::size_t>(*followingPointer) <= reader.Size())
                available = std::min(available, static_cast<std::size_t>(*followingPointer - filePointer));
            if (expected > available)
            {
                result.status = LegacyMapResourceArrayStatus::MalformedAdvisory;
                result.diagnostic = LegacyMapParseDiagnostic{ section, "array elements", static_cast<std::size_t>(filePointer) + 2U,
                    static_cast<std::size_t>(count) * elementBytes, available >= 2U ? available - 2U : 0U, count, false };
                return result;
            }
            entries.reserve(count);
            for (std::uint16_t index = 0; index < count; ++index)
            {
                LegacyMapSpriteFilePosition entry;
                bool spriteRead = false;
                if (spriteIdBytes == 4U)
                {
                    spriteRead = reader.U32(entry.spriteId);
                }
                else
                {
                    std::uint16_t spriteId = 0U;
                    spriteRead = reader.U16(spriteId);
                    entry.spriteId = spriteId;
                }
                if (!spriteRead || !reader.I32(entry.filePosition))
                {
                    result.status = LegacyMapResourceArrayStatus::MalformedAdvisory;
                    result.diagnostic = LegacyMapParseDiagnostic{ section, "spriteId/filePosition", reader.Offset(), elementBytes,
                        reader.Remaining(), index, false };
                    entries.clear();
                    return result;
                }
                entries.push_back(entry);
            }
            result.status = LegacyMapResourceArrayStatus::Embedded;
            return result;
        }

        std::string DiagnosticText(const std::filesystem::path& path, const std::uint64_t fileSize,
            const LegacyMapParseDiagnostic& diagnostic, const LegacyMap& map)
        {
            std::ostringstream stream;
            stream << "map=" << path.filename().string() << " fileSize=" << fileSize
                << " section=" << diagnostic.section << " offset=" << diagnostic.streamOffset
                << " expectedBytes=" << diagnostic.expectedBytes << " remainingBytes=" << diagnostic.remainingBytes
                << " parsedCount=" << diagnostic.parsedCount << " dimensions=" << map.width << 'x' << map.height
                << " sectors=" << map.sectors.size() << " objects=" << map.imageObjects.size()
                << " tileResources=" << map.tileSpriteFilePositions.size()
                << " objectResources=" << map.imageObjectSpriteFilePositions.size()
                << " field=" << diagnostic.field;
            return stream.str();
        }

        void DebugLogDiagnostic(const std::string& message, const bool fatal)
        {
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
            const auto line = std::string(fatal ? "[MapReader][ERROR] " : "[MapReader][WARNING] ") + message + "\r\n";
            OutputDebugStringA(line.c_str());
#else
            (void)message;
            (void)fatal;
#endif
        }
    }

    const LegacyMapSector* LegacyMap::Sector(const std::uint16_t x, const std::uint16_t y) const noexcept
    {
        if (x >= width || y >= height)
        {
            return nullptr;
        }
        const auto index = static_cast<std::size_t>(y) * width + x;
        return index < sectors.size() ? &sectors[index] : nullptr;
    }


    const std::vector<std::size_t>* LegacyMap::SectorImageObjects(
        const std::uint16_t x,
        const std::uint16_t y) const noexcept
    {
        if (x >= width || y >= height)
        {
            return nullptr;
        }
        const auto index = static_cast<std::size_t>(y) * width + x;
        return index < sectorImageObjectIndices.size() ? &sectorImageObjectIndices[index] : nullptr;
    }

    bool LegacyMap::HasTileSprite(const std::uint32_t spriteId) const noexcept
    {
        return std::any_of(tileSpriteFilePositions.begin(), tileSpriteFilePositions.end(),
            [spriteId](const auto& entry) { return entry.spriteId == spriteId; });
    }

    bool LegacyMap::HasImageObjectSprite(const std::uint32_t spriteId) const noexcept
    {
        return std::any_of(imageObjectSpriteFilePositions.begin(), imageObjectSpriteFilePositions.end(),
            [spriteId](const auto& entry) { return entry.spriteId == spriteId; });
    }

    LegacyMapReadResult LegacyMapReader::Load(const std::filesystem::path& path) const
        {
            LegacyMapReadResult result;
            auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = "Map file could not be opened or is empty.";
                OutputDebugStringA("[MAP_DEBUG] return: bytes.empty\n");
            result.error = "EXIT_1";
                return result;
            }

            LegacyBinaryReader reader(bytes);
            LegacyMap map;
            map.originalBytes = std::move(bytes);
            map.path = path;
            map.sizeBytes = reader.Size();

        const auto fail = [&](const std::string& section, const std::string& field,
            const std::size_t expectedBytes, const std::uint64_t parsedCount = 0U)
        {
            LegacyMapParseDiagnostic diagnostic{
                section,
                field,
                reader.Offset(),
                expectedBytes,
                reader.Remaining(),
                parsedCount,
                true
            };
            result.diagnostics.push_back(diagnostic);
            result.error = DiagnosticText(path, map.sizeBytes, diagnostic, map);
            DebugLogDiagnostic(result.error, true);
            std::string dbg = "[MAP_DEBUG] fail: section=" + section + " field=" + field +
                " offset=" + std::to_string(reader.Offset());
            OutputDebugStringA(dbg.c_str());
        };

        std::string failedField;
        std::size_t expectedBytes = 0U;
        if (!ReadZoneHeader(reader, map, failedField, expectedBytes))
        {
            fail("header", failedField, expectedBytes);
            result.error = "EXIT_2";
            return result;
        }

        if (map.version == kSupportedMapVersion)
        {
            map.parserVariant = LegacyMapParserVariant::Map2000;
        }
        else if (map.version == kLayoutCompatibleMapVersion)
        {
            map.parserVariant = LegacyMapParserVariant::Map2024LayoutCompatible;
        }
        else
        {
            fail("header", "unsupported version " + map.version, 0U);
            result.error = "EXIT_3";
            return result;
        }

        if (!reader.I32(map.tileFilePointer))
        {
            fail("map layout", "tile sprite array absolute pointer", 4U);
            result.error = "EXIT_4";
            return result;
        }
        if (!reader.I32(map.imageObjectFilePointer))
        {
            fail("map layout", "image-object sprite array absolute pointer", 4U);
            result.error = "EXIT_5";
            return result;
        }
        if (!reader.U16(map.width))
        {
            fail("map layout", "map width", 2U);
            result.error = "EXIT_6";
            return result;
        }
        if (!reader.U16(map.height))
        {
            fail("map layout", "map height", 2U);
            result.error = "EXIT_7";
            return result;
        }
        if (map.width == 0U || map.height == 0U)
        {
            fail("map layout", map.width == 0U ? "zero map width" : "zero map height", 0U);
            result.error = "EXIT_8";
            return result;
        }

        const auto sectorCount = static_cast<std::size_t>(map.width) * map.height;
        const auto sectorRecordBytes = map.parserVariant == LegacyMapParserVariant::Map2024LayoutCompatible ? 6U : 4U;
        if (sectorCount > reader.Remaining() / sectorRecordBytes)
        {
            fail("sectors", "sector array", sectorCount * sectorRecordBytes, 0U);
            result.error = "EXIT_9";
            return result;
        }

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
        map.sectorDataFileOffset = reader.Offset();
#endif
        map.sectors.reserve(sectorCount);
        for (std::size_t i = 0; i < sectorCount; ++i)
        {
            LegacyMapSector sector;
            bool spriteRead = false;
            if (map.parserVariant == LegacyMapParserVariant::Map2024LayoutCompatible)
            {
                spriteRead = reader.U32(sector.spriteId);
            }
            else
            {
                std::uint16_t spriteId = 0U;
                spriteRead = reader.U16(spriteId);
                sector.spriteId = spriteId;
            }
            if (!spriteRead)
            {
                fail("sectors", "sector spriteId", sectorRecordBytes - 2U, i);
            result.error = "EXIT_10";
                return result;
            }
            if (!reader.U8(sector.property))
            {
                fail("sectors", "sector property", 1U, i);
            result.error = "EXIT_11";
                return result;
            }
            if (!reader.U8(sector.light))
            {
                fail("sectors", "sector light", 1U, i);
            result.error = "EXIT_12";
                return result;
            }
            map.sectors.push_back(sector);
        }

        std::int32_t imageObjectCount = 0;
        if (!reader.I32(imageObjectCount))
        {
            fail("image objects", "object count", 4U);
            result.error = "EXIT_13";
            return result;
        }
        if (imageObjectCount < 0)
        {
            fail("image objects", "negative object count", 0U, static_cast<std::uint32_t>(imageObjectCount));
            result.error = "EXIT_14";
            return result;
        }
        constexpr std::size_t minimumObjectRecordBytes = 30U;
        if (static_cast<std::size_t>(imageObjectCount) > reader.Remaining() / minimumObjectRecordBytes)
        {
            fail("image objects", "object records", static_cast<std::size_t>(imageObjectCount) * minimumObjectRecordBytes,
                static_cast<std::uint32_t>(imageObjectCount));
            result.error = "EXIT_15";
            return result;
        }

        map.imageObjects.reserve(static_cast<std::size_t>(imageObjectCount));
        map.sectorImageObjectIndices.resize(sectorCount);
        for (std::int32_t i = 0; i < imageObjectCount; ++i)
        {
            LegacyMapImageObject object;
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
            object.sourceFileOffset = reader.Offset();
#endif
            failedField.clear();
            expectedBytes = 0U;
            if (!ReadImageObject(reader, object, failedField, expectedBytes))
            {
                fail("image objects", failedField, expectedBytes, static_cast<std::uint32_t>(i));
            result.error = "EXIT_16";
                return result;
            }
            std::uint64_t parsedPositions = 0U;
            if (!ReadPositionList(reader, object, failedField, expectedBytes, parsedPositions))
            {
                fail("image-object positions", failedField, expectedBytes, parsedPositions);
            result.error = "EXIT_17";
                return result;
            }
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
            object.sourceRecordSize = reader.Offset() - object.sourceFileOffset;
#endif
            const auto objectIndex = map.imageObjects.size();
                                    // Add all sector positions from positions vector - this is the primary source of sector coordinates
                                    // The x/y fields are wallDirection/wallValue for wall objects (type 3,4), 
                                    // but for other object types they encode the primary sector position.
                                    // Use positions vector first, then fallback to x,y for non-wall objects.
                                    bool hasPositions = false;
                                    for (const auto& [posX, posY] : object.positions)
                                    {
                                        if (posX < map.width && posY < map.height) {
                                            map.sectorImageObjectIndices[static_cast<std::size_t>(posY) * map.width + posX].push_back(objectIndex);
                                            hasPositions = true;
                                        }
                                    }
                                    // Fallback: if no positions vector data, use x,y as primary sector coordinates
                                    // for non-wall object types (x,y are sector coordinates for non-wall objects)
                                    if (!hasPositions && (object.wrapperType != kTypeImageObject && object.wrapperType != kTypeShadowObject)) {
                                        if (object.x < map.width && object.y < map.height) {
                                            map.sectorImageObjectIndices[static_cast<std::size_t>(object.y) * map.width + object.x].push_back(objectIndex);
                                        }
                                    }
                                    map.imageObjects.push_back(std::move(object));
        }

        const auto structuralEnd = reader.Offset();
        map.bytesConsumed = structuralEnd;
        const auto tileArray = ReadSpriteFilePositionArray(reader, map.tileFilePointer, structuralEnd,
            map.imageObjectFilePointer, "tile sprite file-position array",
            map.parserVariant == LegacyMapParserVariant::Map2024LayoutCompatible ? 4U : 2U,
            map.tileSpriteFilePositions);
        const auto objectArray = ReadSpriteFilePositionArray(reader, map.imageObjectFilePointer, structuralEnd,
            std::nullopt, "image-object sprite file-position array", 2U, map.imageObjectSpriteFilePositions);
        map.tileResourceArrayStatus = tileArray.status;
        map.imageObjectResourceArrayStatus = objectArray.status;

        const auto appendAdvisoryDiagnostic = [&](const SpriteArrayReadResult& arrayResult)
        {
            if (!arrayResult.diagnostic) return;
            map.diagnostics.push_back(*arrayResult.diagnostic);
            result.diagnostics.push_back(*arrayResult.diagnostic);
            DebugLogDiagnostic(DiagnosticText(path, map.sizeBytes, *arrayResult.diagnostic, map), false);
        };
        appendAdvisoryDiagnostic(tileArray);
        appendAdvisoryDiagnostic(objectArray);

        result.map = std::move(map);
        {
            std::string dbg = "[MAP_DEBUG] success: width=" + std::to_string(result.map->width) +
                " height=" + std::to_string(result.map->height) +
                " sectors=" + std::to_string(result.map->sectors.size()) +
                " objects=" + std::to_string(result.map->imageObjects.size()) +
                " error.size=" + std::to_string(result.error.size()) +
                " diagnostics=" + std::to_string(result.diagnostics.size());
            OutputDebugStringA(dbg.c_str());
        }
        // EXIT_18 burada olmamali — basari yolunda error set edilirse
        // advisory sprite-array diagnostic varken Succeeded() false doner.
        return result;
    }
}
