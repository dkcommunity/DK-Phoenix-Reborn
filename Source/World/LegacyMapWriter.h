#pragma once

#include "World/LegacyMapReader.h"

#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::world
{
    struct LegacyMapWriteResult
    {
        bool succeeded{ false };
        bool backupCreated{ false };
        bool originalBytesPreserved{ false };
        std::filesystem::path backupPath;
        std::string error;
    };

    struct LegacyMapSerializationResult
    {
        bool succeeded{ false };
        bool originalBytesPreserved{ false };
        std::vector<std::uint8_t> bytes;
        std::string error;
    };

    class LegacyMapWriter final
    {
    public:
        [[nodiscard]] LegacyMapSerializationResult Serialize(const LegacyMap& map) const;
        [[nodiscard]] LegacyMapWriteResult Save(const LegacyMap& map, const std::filesystem::path& path) const;
    };
}
