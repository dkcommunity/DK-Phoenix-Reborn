#include "Content/LegacyAssetResolver.h"

#include <algorithm>
#include <cwctype>

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] std::wstring LowerWide(std::wstring value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](const wchar_t ch) {
                return static_cast<wchar_t>(std::towlower(ch));
            });
            return value;
        }
    }

    const wchar_t* LegacyAssetResolver::IndexedSpritePackFileDefKey(
        const std::wstring_view packName,
        const std::wstring_view extension,
        const bool index) noexcept
    {
        const auto pack = LowerWide(std::wstring(packName));
        const auto ext = LowerWide(std::wstring(extension));

        if (ext == L"ispk")
        {
            if (pack == L"creature") return index ? L"FILE_ISPRITEINDEX_CREATURE" : L"FILE_ISPRITE_CREATURE";
            if (pack == L"creature2") return index ? L"FILE_ISPRITEINDEX_CREATURE2" : L"FILE_ISPRITE_CREATURE2";
            if (pack == L"addonman") return index ? L"FILE_ISPRITEINDEX_ADDON_MAN" : L"FILE_ISPRITE_ADDON_MAN";
            if (pack == L"addonwoman") return index ? L"FILE_ISPRITEINDEX_ADDON_WOMAN" : L"FILE_ISPRITE_ADDON_WOMAN";
            if (pack == L"ousters") return index ? L"FILE_ISPRITEINDEX_OUSTERS" : L"FILE_ISPRITE_OUSTERS";
            if (pack == L"vampire") return index ? L"FILE_ISPRITEINDEX_VAMPIRE" : L"FILE_ISPRITE_VAMPIRE";
            if (pack == L"itemdrop") return index ? L"FILE_ISPRITEINDEX_ITEMDROP" : L"FILE_ISPRITE_ITEMDROP";
        }

        if (ext == L"sspk")
        {
            if (pack == L"creature" || pack == L"creatureshadow") return index ? L"FILE_SSPRITEINDEX_CREATURE" : L"FILE_SSPRITE_CREATURE";
            if (pack == L"creature2" || pack == L"creatureshadow2") return index ? L"FILE_SSPRITEINDEX_CREATURE2" : L"FILE_SSPRITE_CREATURE2";
            if (pack == L"addonman" || pack == L"addonmanshadow") return index ? L"FILE_SSPRITEINDEX_ADDON_MAN" : L"FILE_SSPRITE_ADDON_MAN";
            if (pack == L"addonwoman" || pack == L"addonwomanshadow") return index ? L"FILE_SSPRITEINDEX_ADDON_WOMAN" : L"FILE_SSPRITE_ADDON_WOMAN";
            if (pack == L"ousters" || pack == L"oustersshadow") return index ? L"FILE_SSPRITEINDEX_OUSTERS" : L"FILE_SSPRITE_OUSTERS";
            if (pack == L"vampire" || pack == L"vampireshadow") return index ? L"FILE_SSPRITEINDEX_VAMPIRE" : L"FILE_SSPRITE_VAMPIRE";
        }

        return L"";
    }

    const wchar_t* LegacyAssetResolver::CreatureFramePackFileDefKey(
        const std::wstring_view packName,
        const bool index) noexcept
    {
        const auto pack = LowerWide(std::wstring(packName));
        if (pack == L"creature") return index ? L"FILE_CFRAMEINDEX_CREATURE" : L"FILE_CFRAME_CREATURE";
        if (pack == L"creatureshadow") return index ? L"FILE_CFRAMEINDEX_CREATURE_SHADOW" : L"FILE_CFRAME_CREATURE_SHADOW";
        if (pack == L"creature2") return index ? L"FILE_CFRAMEINDEX_CREATURE2" : L"FILE_CFRAME_CREATURE2";
        if (pack == L"creatureshadow2") return index ? L"FILE_CFRAMEINDEX_CREATURE_SHADOW2" : L"FILE_CFRAME_CREATURE_SHADOW2";
        if (pack == L"addonman") return index ? L"FILE_CFRAMEINDEX_ADDON_MAN" : L"FILE_CFRAME_ADDON_MAN";
        if (pack == L"addonmanshadow") return index ? L"FILE_CFRAMEINDEX_ADDON_MAN_SHADOW" : L"FILE_CFRAME_ADDON_MAN_SHADOW";
        if (pack == L"addonwoman") return index ? L"FILE_CFRAMEINDEX_ADDON_WOMAN" : L"FILE_CFRAME_ADDON_WOMAN";
        if (pack == L"addonwomanshadow") return index ? L"FILE_CFRAMEINDEX_ADDON_WOMAN_SHADOW" : L"FILE_CFRAME_ADDON_WOMAN_SHADOW";
        if (pack == L"ousters") return index ? L"FILE_CFRAMEINDEX_OUSTERS" : L"FILE_CFRAME_OUSTERS";
        if (pack == L"oustersshadow") return index ? L"FILE_CFRAMEINDEX_OUSTERS_SHADOW" : L"FILE_CFRAME_OUSTERS_SHADOW";
        if (pack == L"vampire") return index ? L"FILE_CFRAMEINDEX_VAMPIRE" : L"FILE_CFRAME_VAMPIRE";
        if (pack == L"vampireshadow") return index ? L"FILE_CFRAMEINDEX_VAMPIRE_SHADOW" : L"FILE_CFRAME_VAMPIRE_SHADOW";
        return L"";
    }

    std::wstring LegacyAssetResolver::ImagePackFallback(
        const std::wstring_view packName,
        const std::wstring_view extension,
        const bool index)
    {
        std::wstring fallback = L"Image\\";
        fallback += std::wstring(packName);
        fallback += L".";
        fallback += std::wstring(extension);
        if (index)
        {
            fallback += L"i";
        }
        return fallback;
    }

    std::wstring LegacyAssetResolver::CreatureSpritePackNameForBody(
        const std::uint32_t body,
        const bool shadow)
    {
        if (shadow)
        {
            if (body < 287U)
            {
                return L"creature";
            }
            return body < 409U ? L"creature2" : L"creature3";
        }

        if (body == 473U)
        {
            return L"creature7";
        }
        if (body < 287U)
        {
            return L"creature";
        }
        return body < 409U ? L"creature2" : L"creature3";
    }
}
