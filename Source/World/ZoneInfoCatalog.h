#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::world
{
    struct ZoneInfo
    {
        std::uint16_t id{ 0 };
        std::string name;
        std::uint8_t property{ 0 };
        std::uint16_t musicId{ 0 };
        std::string filename;
        std::string infoFilename;
        std::string teenFilename;
        bool safety{ false };
        std::uint16_t creatureColorSet{ 0xFFFFU };
        std::uint16_t minimapSpriteId{ 0 };
        bool holyLand{ false };
        bool chatMaskByRace{ false };
        bool cannotAttackInSafe{ false };
        bool cannotUseSpecialItem{ false };
        bool competenceZone{ false };
        std::uint8_t pkType{ 0 };
        std::vector<std::uint16_t> soundIds;
        std::vector<std::string> mapShapes;
    };

    struct ZoneInfoCatalogResult
    {
        std::filesystem::path path;
        std::vector<ZoneInfo> zones;
        std::string error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty();
        }
    };

    class ZoneInfoCatalog final
    {
    public:
        [[nodiscard]] ZoneInfoCatalogResult Load(const std::filesystem::path& path) const;
        [[nodiscard]] static std::optional<ZoneInfo> Find(const ZoneInfoCatalogResult& catalog, std::uint16_t zoneId);
    };
}
