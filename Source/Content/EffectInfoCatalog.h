#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::content
{
    enum class EffectBltType : std::uint8_t
    {
        Normal = 0,
        Effect = 1,
        Shadow = 2,
        Screen = 3
    };

    struct EffectSpriteInfo
    {
        std::uint16_t effectId{ 0 };
        EffectBltType bltType{ EffectBltType::Effect };
        std::uint16_t frameId{ 0xFFFFU };
        bool repeatFrame{ false };
        bool pairFrameBack{ false };
        std::uint16_t actionEffectFrameId{ 0xFFFFU };
        std::uint16_t femaleEffectSpriteType{ 0xFFFFU };
        std::vector<std::uint16_t> pairFrameIds;
    };

    struct EffectInfoCatalogResult
    {
        std::vector<EffectSpriteInfo> effects;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && !effects.empty();
        }

        [[nodiscard]] const EffectSpriteInfo* Find(std::uint16_t effectId) const noexcept;
    };

    class EffectInfoCatalog final
    {
    public:
        [[nodiscard]] EffectInfoCatalogResult Load(const std::filesystem::path& path) const;
    };
}
