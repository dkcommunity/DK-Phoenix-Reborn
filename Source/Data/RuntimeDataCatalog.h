#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace phoenix::data
{
    struct DataFolderSummary
    {
        std::wstring name;
        std::uint64_t fileCount{ 0 };
        std::uint64_t totalBytes{ 0 };
    };

    struct DataExtensionSummary
    {
        std::wstring extension;
        std::uint64_t fileCount{ 0 };
        std::uint64_t totalBytes{ 0 };
    };

    struct RuntimeDataInventory
    {
        std::filesystem::path root;
        std::vector<DataFolderSummary> folders;
        std::vector<DataExtensionSummary> extensions;

        [[nodiscard]] bool HasExtension(const std::wstring& extension) const;
    };

    class RuntimeDataCatalog final
    {
    public:
        [[nodiscard]] RuntimeDataInventory Scan(const std::filesystem::path& root) const;
    };
}
