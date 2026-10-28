#include "UI/Text/FontManager.h"

namespace phoenix::ui::text
{
    bool FontManager::LoadManifest(const std::filesystem::path& manifestPath)
    {
        return manifest_.Load(manifestPath);
    }

    bool FontManager::LoadFontStack(const std::filesystem::path& stackPath)
    {
        FontStackConfigLoader loader;
        return loader.Load(stackPath, stack_);
    }

    const FontStackConfig& FontManager::GetDefaultStack() const noexcept
    {
        return stack_;
    }

    bool FontManager::HasFontFamily(const std::string_view familyId) const
    {
        return manifest_.FindFamily(familyId).has_value();
    }

    std::optional<std::filesystem::path> FontManager::FindLocalFontFile(const std::string_view familyId) const
    {
        const auto family = manifest_.FindFamily(familyId);
        if (!family || family->localFiles.empty())
        {
            return std::nullopt;
        }

        const auto& path = family->localFiles.front().path;
        if (!std::filesystem::exists(path))
        {
            return std::nullopt;
        }

        return path;
    }
}
