#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace phoenix::world
{
    struct MapFileProbeResult
    {
        std::filesystem::path path;
        std::uint64_t sizeBytes{ 0 };
        std::vector<std::uint32_t> firstDwords;
        bool exists{ false };
    };

    class MapFileProbe final
    {
    public:
        [[nodiscard]] MapFileProbeResult Probe(const std::filesystem::path& path) const;
    };
}
