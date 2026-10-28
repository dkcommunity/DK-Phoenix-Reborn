#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::world
{
    struct LegacyMap;

    inline constexpr std::uint8_t kLegacyBlockUnderground = 0x01U;
    inline constexpr std::uint8_t kLegacyBlockGround = 0x02U;
    inline constexpr std::uint8_t kLegacyBlockFlying = 0x04U;
    inline constexpr std::uint8_t kLegacyServerPortal = 0x80U;
    inline constexpr std::uint8_t kLegacySafetySlayer = 0x01U;
    inline constexpr std::uint8_t kLegacySafetyVampire = 0x02U;
    inline constexpr std::uint8_t kLegacySafetyCommon = 0x04U;
    inline constexpr std::uint8_t kLegacySafetyOusters = 0x10U;

    struct LegacyMapPortalTarget
    {
        std::uint16_t zoneId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        bool destinationKnown{ false };

    };

    struct LegacyMapPortal
    {
        std::uint8_t type{ 0 };
        std::vector<LegacyMapPortalTarget> targets;

    };

    struct LegacyMapServerSector
    {
        std::uint8_t preservedServerFlags{ 0 };
        std::uint8_t safetyFlags{ 0 };
        std::optional<LegacyMapPortal> portal;

    };

    struct LegacyMapSidecarData
    {
        std::vector<LegacyMapServerSector> sectors;
        std::vector<std::uint8_t> mipTrailingBytes;
        std::vector<std::uint8_t> smpTrailingBytes;
        std::vector<std::uint8_t> ssiTrailingBytes;
        std::vector<std::uint8_t> originalMipBytes;
        std::vector<std::uint8_t> originalSmpBytes;
        std::vector<std::uint8_t> originalSsiBytes;
        bool mipLoaded{ false };
        bool smpLoaded{ false };
        bool ssiLoaded{ false };
        bool modified{ false };
    };

    struct LegacyMapSidecarPaths
    {
        std::filesystem::path mip;
        std::filesystem::path smp;
        std::filesystem::path ssi;

        [[nodiscard]] static LegacyMapSidecarPaths FromMapPath(const std::filesystem::path& mapPath);
    };

    struct LegacyMapSidecarResult
    {
        bool succeeded{ false };
        std::string error;
    };

    struct LegacyMapBundleWriteResult
    {
        bool succeeded{ false };
        bool backupCreated{ false };
        bool originalMapBytesPreserved{ false };
        std::string error;
        std::vector<std::filesystem::path> backupPaths;
        // Portal'lar icin hedef X/Y atamasi eksik (destinationKnown=false) olan
        // sektorlerin (x, y) koordinatlari. Save'i engellemez; MapEditor bunu
        // kullaniciya bir warning olarak gosterir.
        std::vector<std::pair<std::uint16_t, std::uint16_t>> portalDestinationsUnknown;
    };

    class LegacyMapSidecarReader final
    {
    public:
        [[nodiscard]] LegacyMapSidecarResult Load(LegacyMap& map, const std::filesystem::path& mapPath) const;
    };

    class LegacyMapBundleWriter final
    {
    public:
        [[nodiscard]] LegacyMapBundleWriteResult Save(const LegacyMap& map, const std::filesystem::path& mapPath) const;
    };

    [[nodiscard]] LegacyMapServerSector* LegacyServerSector(LegacyMap& map, std::uint16_t x, std::uint16_t y) noexcept;
    [[nodiscard]] const LegacyMapServerSector* LegacyServerSector(
        const LegacyMap& map, std::uint16_t x, std::uint16_t y) noexcept;
    void InitializeLegacyMapSidecars(LegacyMap& map);
}
