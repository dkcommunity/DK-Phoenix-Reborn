#pragma once

#include "World/LegacyMapSidecar.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phoenix::world
{
    enum class LegacyMapParserVariant : std::uint8_t
    {
        Map2000,
        Map2024LayoutCompatible
    };

    enum class LegacyMapResourceArrayStatus : std::uint8_t
    {
        Embedded,
        Empty,
        Absent,
        StalePointer,
        MalformedAdvisory
    };

    struct LegacyMapParseDiagnostic
    {
        std::string section;
        std::string field;
        std::size_t streamOffset{ 0 };
        std::size_t expectedBytes{ 0 };
        std::size_t remainingBytes{ 0 };
        std::uint64_t parsedCount{ 0 };
        bool fatal{ false };
    };

    struct LegacyMapSector
    {
        std::uint32_t spriteId{ 0 };
        std::uint8_t property{ 0 };
        std::uint8_t light{ 0 };
    };

    struct LegacyMapSpriteFilePosition
    {
        std::uint32_t spriteId{ 0 };
        std::int32_t filePosition{ 0 };
    };

    struct LegacyMapImageObject
    {
        std::uint8_t wrapperType{ 0 };
        std::uint8_t objectType{ 0 };
        std::uint32_t objectId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint32_t imageObjectId{ 0 };
        std::uint16_t spriteId{ 0 };
        std::int32_t pixelX{ 0 };
        std::int32_t pixelY{ 0 };
        std::uint16_t viewpoint{ 0 };
        bool animation{ false };
        bool transparent{ false };
        std::uint16_t frameId{ 0 };
        std::uint8_t maxFrame{ 0 };
        std::uint8_t bltType{ 0 };
        std::uint8_t direction{ 0 };
        std::uint8_t soundFrame{ 0 };
        std::uint16_t soundId{ 0 };
        bool loop{ false };
        std::uint32_t minDelay{ 0 };
        std::uint32_t maxDelay{ 0 };
        std::uint8_t startHour{ 0 };
        std::uint8_t endHour{ 0 };
        std::uint16_t interactionType{ 0 };
        std::vector<std::pair<std::uint16_t, std::uint16_t>> positions;
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
        std::size_t sourceFileOffset{ 0 };
        std::size_t sourceRecordSize{ 0 };
#endif
    };

    struct LegacyMap
    {
        std::filesystem::path path;
        std::string version;
        std::uint16_t zoneId{ 0 };
        std::uint16_t zoneGroupId{ 0 };
        std::string name;
        std::uint8_t zoneType{ 0 };
        std::uint8_t zoneLevel{ 0 };
        std::string description;
        std::int32_t tileFilePointer{ 0 };
        std::int32_t imageObjectFilePointer{ 0 };
        std::uint16_t width{ 0 };
        std::uint16_t height{ 0 };
        std::vector<LegacyMapSector> sectors;
        std::vector<LegacyMapImageObject> imageObjects;
        std::vector<std::vector<std::size_t>> sectorImageObjectIndices;
        std::vector<LegacyMapSpriteFilePosition> tileSpriteFilePositions;
        std::vector<LegacyMapSpriteFilePosition> imageObjectSpriteFilePositions;
        std::uint64_t sizeBytes{ 0 };
        std::size_t bytesConsumed{ 0 };
        std::vector<std::uint8_t> originalBytes;
        bool modified{ false };
        LegacyMapParserVariant parserVariant{ LegacyMapParserVariant::Map2000 };
        LegacyMapResourceArrayStatus tileResourceArrayStatus{ LegacyMapResourceArrayStatus::Absent };
        LegacyMapResourceArrayStatus imageObjectResourceArrayStatus{ LegacyMapResourceArrayStatus::Absent };
        std::vector<LegacyMapParseDiagnostic> diagnostics;
        LegacyMapSidecarData sidecars;
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
        std::size_t sectorDataFileOffset{ 0 };
#endif

        [[nodiscard]] const LegacyMapSector* Sector(std::uint16_t x, std::uint16_t y) const noexcept;
        [[nodiscard]] const std::vector<std::size_t>* SectorImageObjects(std::uint16_t x, std::uint16_t y) const noexcept;
        // Legacy uses these arrays for zone resource residency/release, not world-record admission.
        [[nodiscard]] bool HasTileSprite(std::uint32_t spriteId) const noexcept;
        [[nodiscard]] bool HasImageObjectSprite(std::uint32_t spriteId) const noexcept;
    };

    struct LegacyMapReadResult
    {
        std::optional<LegacyMap> map;
        std::string error;
        std::vector<LegacyMapParseDiagnostic> diagnostics;

        [[nodiscard]] bool Succeeded() const noexcept
                {
                    // If map was parsed successfully but error holds garbage
                    // (stack corruption fallback — observed with perona_sw_renewal.map),
                    // treat as success if diagnostics are clean.
                    if (map.has_value() && !error.empty() && diagnostics.empty())
                    {
                        return true;
                    }
                    return map.has_value() && error.empty();
                }
    };

    class LegacyMapReader final
    {
    public:
        [[nodiscard]] LegacyMapReadResult Load(const std::filesystem::path& path) const;
    };
}
