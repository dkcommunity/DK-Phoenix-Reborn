#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct EffectStatusInfo
    {
        std::uint16_t effectStatusId{ 0 };
        bool useEffectSprite{ true };
        bool attachGround{ false };
        std::uint8_t new201{ 0 };
        std::uint16_t effectSpriteType{ 0xFFFFU };
        std::uint16_t effectColor{ 0xFFFFU };
        std::uint8_t effectColorPart{ 0 };
        std::uint16_t actionInfo{ 0xFFFFU };
        std::uint16_t originalActionInfo{ 0xFFFFU };
        std::int32_t soundId{ -1 };
    };

    struct EffectStatusCatalogResult
    {
        std::vector<EffectStatusInfo> statuses;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && !statuses.empty();
        }

        [[nodiscard]] const EffectStatusInfo* Find(std::uint16_t effectStatusId) const noexcept;
    };

    class EffectStatusCatalog final
    {
    public:
        [[nodiscard]] EffectStatusCatalogResult Load(const std::filesystem::path& path) const;
    };
}
