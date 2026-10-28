#pragma once

#include "UI/Text/FontAssetManifest.h"
#include "UI/Text/FontStackConfig.h"

#include <filesystem>
#include <optional>
#include <string_view>

namespace phoenix::ui::text
{
    class FontManager final
    {
    public:
        bool LoadManifest(const std::filesystem::path& manifestPath);
        bool LoadFontStack(const std::filesystem::path& stackPath);

        [[nodiscard]] const FontStackConfig& GetDefaultStack() const noexcept;
        [[nodiscard]] bool HasFontFamily(std::string_view familyId) const;
        [[nodiscard]] std::optional<std::filesystem::path> FindLocalFontFile(std::string_view familyId) const;

    private:
        FontAssetManifest manifest_;
        FontStackConfig stack_;
    };
}
