#pragma once

#include "World/LegacyMapReader.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::tools::map_editor
{
    inline constexpr std::uint16_t kMapPresetFormatVersion = 1U;

    struct MapPresetRect
    {
        std::uint16_t left{ 0 };
        std::uint16_t top{ 0 };
        std::uint16_t right{ 0 };
        std::uint16_t bottom{ 0 };

        [[nodiscard]] bool IsValid() const noexcept { return left <= right && top <= bottom; }
    };

    struct MapPresetResource
    {
        bool present{ false };
        std::uint32_t spriteId{ 0 };
        std::int32_t filePosition{ 0 };
    };

    struct MapPresetTile
    {
        std::int32_t relativeX{ 0 };
        std::int32_t relativeY{ 0 };
        world::LegacyMapSector sector;
        MapPresetResource resource;
    };

    struct MapPresetObject
    {
        world::LegacyMapImageObject object;
        std::int32_t relativePixelX{ 0 };
        std::int32_t relativePixelY{ 0 };
        std::int32_t relativeViewpoint{ 0 };
        bool preBackground{ false };
        std::vector<std::pair<std::int32_t, std::int32_t>> relativePositions;
        MapPresetResource resource;
    };

    struct MapPreset
    {
        std::uint16_t formatVersion{ kMapPresetFormatVersion };
        std::string name;
        std::int32_t originX{ 0 };
        std::int32_t originY{ 0 };
        std::uint16_t width{ 0 };
        std::uint16_t height{ 0 };
        std::vector<MapPresetTile> tiles;
        std::vector<MapPresetObject> objects;
    };

    struct MapPresetResult
    {
        bool succeeded{ false };
        std::string error;
    };

    struct MapPresetLoadResult
    {
        std::optional<MapPreset> preset;
        std::string error;

        [[nodiscard]] bool Succeeded() const noexcept { return preset.has_value() && error.empty(); }
    };

    struct MapPresetCaptureResult
    {
        std::optional<MapPreset> preset;
        std::string error;

        [[nodiscard]] bool Succeeded() const noexcept { return preset.has_value() && error.empty(); }
    };

    struct MapPresetPlacementResult
    {
        bool succeeded{ false };
        std::vector<std::uint32_t> objectIds;
        std::string error;
    };

    [[nodiscard]] MapPresetCaptureResult CaptureMapPreset(
        const world::LegacyMap& map, const MapPresetRect& selection, std::string name);
    [[nodiscard]] MapPresetResult ValidateMapPresetPlacement(
        const world::LegacyMap& map, const MapPreset& preset, std::uint16_t targetX, std::uint16_t targetY);
    [[nodiscard]] MapPresetPlacementResult PlaceMapPreset(
        world::LegacyMap& map, const MapPreset& preset, std::uint16_t targetX, std::uint16_t targetY);

    [[nodiscard]] MapPresetResult SaveMapPreset(
        const MapPreset& preset, const std::filesystem::path& path);
    [[nodiscard]] MapPresetLoadResult LoadMapPreset(const std::filesystem::path& path);
    [[nodiscard]] MapPresetResult ValidateMapPresetName(const std::string& name);
}
