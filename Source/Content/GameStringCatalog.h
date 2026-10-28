#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct GameStringCatalogResult
    {
        std::vector<std::string> strings;
        std::vector<std::string> nicknames;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && (!strings.empty() || !nicknames.empty());
        }

        [[nodiscard]] std::optional<std::string> FindString(std::uint32_t index) const;
        [[nodiscard]] std::optional<std::string> FindNickname(std::uint32_t index) const;
    };

    class GameStringCatalog final
    {
    public:
        [[nodiscard]] GameStringCatalogResult Load(
            const std::filesystem::path& stringPath,
            const std::filesystem::path& nicknamePath) const;
    };
}
