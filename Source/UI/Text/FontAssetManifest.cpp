#include "UI/Text/FontAssetManifest.h"

namespace phoenix::ui::text
{
    namespace
    {
        std::vector<FontFamilyRecord> DefaultManifestFamilies(const std::filesystem::path& projectRoot)
        {
            return {
                FontFamilyRecord{ "noto_sans", "Noto Sans", "primary_ui", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSans/NotoSans[wdth,wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_italic", "Noto Sans Italic", "primary_ui_italic", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSans/NotoSans-Italic[wdth,wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_kr", "Noto Sans KR", "fallback_korean", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansCJK/KR/NotoSansKR[wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_jp", "Noto Sans JP", "fallback_japanese", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansCJK/JP/NotoSansJP[wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_sc", "Noto Sans SC", "fallback_simplified_chinese", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansCJK/SC/NotoSansSC[wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_tc", "Noto Sans TC", "fallback_traditional_chinese", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansCJK/TC/NotoSansTC[wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_hk", "Noto Sans HK", "fallback_hong_kong", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansCJK/HK/NotoSansHK[wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_symbols2", "Noto Sans Symbols 2", "fallback_symbols", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansSymbols2/NotoSansSymbols2-Regular.ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "noto_sans_mono", "Noto Sans Mono", "debug_monospace", false, { FontFileRecord{ projectRoot / "Assets/Fonts/Noto/NotoSansMono/NotoSansMono[wdth,wght].ttf", {}, {}, 0 } } },
                FontFamilyRecord{ "segoe_ui_emoji", "Segoe UI Emoji", "system_emoji", true, {} },
            };
        }
    }

    bool FontAssetManifest::Load(const std::filesystem::path& manifestPath)
    {
        if (!std::filesystem::exists(manifestPath))
        {
            loaded_ = false;
            families_.clear();
            return false;
        }

        sourcePath_ = manifestPath;
        families_ = DefaultManifestFamilies(manifestPath.parent_path().parent_path().parent_path().parent_path());
        loaded_ = true;
        return true;
    }

    bool FontAssetManifest::IsLoaded() const noexcept
    {
        return loaded_;
    }

    const std::filesystem::path& FontAssetManifest::SourcePath() const noexcept
    {
        return sourcePath_;
    }

    const std::vector<FontFamilyRecord>& FontAssetManifest::Families() const noexcept
    {
        return families_;
    }

    std::optional<FontFamilyRecord> FontAssetManifest::FindFamily(const std::string_view familyId) const
    {
        for (const auto& family : families_)
        {
            if (family.id == familyId)
            {
                return family;
            }
        }

        return std::nullopt;
    }
}
