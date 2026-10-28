#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::assets
{
    struct InfBinarySample
    {
        std::filesystem::path path;
        std::uint64_t sizeBytes{ 0 };
        std::uint32_t firstDword{ 0 };
        std::vector<std::string> printableStrings;
        bool exists{ false };
        bool likelyBinary{ false };
    };

    class InfBinarySchemaReader final
    {
    public:
        [[nodiscard]] InfBinarySample Sample(const std::filesystem::path& path) const;
    };
}
