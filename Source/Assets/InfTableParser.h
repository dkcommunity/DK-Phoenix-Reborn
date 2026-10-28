#pragma once

#include "Assets/LegacyAssetTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::assets
{
    struct InfRecord
    {
        std::uint32_t lineNumber{ 0 };
        std::vector<std::string> fields;
    };

    struct InfTable
    {
        std::filesystem::path sourcePath;
        std::vector<InfRecord> records;
    };

    class InfTableParser final
    {
    public:
        [[nodiscard]] LegacyReadResult<InfTable> Read(const std::filesystem::path& path) const;
    };
}
