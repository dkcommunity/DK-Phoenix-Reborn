#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct ItemOptionInfo
    {
        std::uint16_t optionId{ 0 };
        std::string englishName;
        std::string displayName;
        std::int32_t part{ 0 };
        std::int32_t plusPoint{ 0 };
        std::int32_t priceMultiplier{ 0 };
        std::int32_t requireStr{ 0 };
        std::int32_t requireDex{ 0 };
        std::int32_t requireInt{ 0 };
        std::int32_t requireSum{ 0 };
        std::int32_t requireLevel{ 0 };
        std::int32_t colorSet{ 0 };
        std::int32_t upgradeOptionType{ 0 };
        std::int32_t previousOptionType{ 0 };
    };

    struct ItemOptionPartName
    {
        std::string englishName;
        std::string displayName;
    };

    struct ItemOptionCatalogResult
    {
        std::vector<ItemOptionPartName> partNames;
        std::map<std::uint16_t, ItemOptionInfo> options;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && !options.empty();
        }

        [[nodiscard]] const ItemOptionInfo* Find(std::uint16_t optionId) const noexcept;
    };

    class ItemOptionCatalog final
    {
    public:
        [[nodiscard]] ItemOptionCatalogResult Load(const std::filesystem::path& path) const;
    };
}
