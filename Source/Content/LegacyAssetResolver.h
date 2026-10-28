#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace phoenix::content
{
    class LegacyAssetResolver final
    {
    public:
        [[nodiscard]] static const wchar_t* IndexedSpritePackFileDefKey(
            std::wstring_view packName,
            std::wstring_view extension,
            bool index) noexcept;

        [[nodiscard]] static const wchar_t* CreatureFramePackFileDefKey(
            std::wstring_view packName,
            bool index) noexcept;

        [[nodiscard]] static std::wstring ImagePackFallback(
            std::wstring_view packName,
            std::wstring_view extension,
            bool index);

        [[nodiscard]] static std::wstring CreatureSpritePackNameForBody(
            std::uint32_t body,
            bool shadow);
    };
}
