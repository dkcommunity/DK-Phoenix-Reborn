#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace phoenix::content
{
    struct FileDefCatalogResult
    {
        std::filesystem::path dataRoot;
        std::map<std::wstring, std::filesystem::path> entries;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && !entries.empty();
        }

        [[nodiscard]] std::optional<std::filesystem::path> Find(std::wstring_view key) const;
        [[nodiscard]] std::filesystem::path Resolve(std::wstring_view key, std::wstring_view fallbackRelativePath) const;
    };

    class FileDefCatalog final
    {
    public:
        [[nodiscard]] FileDefCatalogResult Load(const std::filesystem::path& dataRoot) const;
    };
}
