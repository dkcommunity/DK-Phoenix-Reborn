#pragma once

#include "Content/LegacyUiSpriteCatalog.h"
#include "Graphics/ImageData.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::ui
{
    enum class LegacyUiSpriteLoadMode
    {
        RuntimeInventory,
        FullAudit
    };

    struct LegacyUiSpriteSmokeFrame
    {
        std::string key;
        content::LegacyUiSpriteScope scope{ content::LegacyUiSpriteScope::Global };
        content::LegacyUiRace race{ content::LegacyUiRace::Shared };
        std::uint16_t frameIndex{ 0 };
        graphics::ImageData image;
    };

    struct LegacyUiSpriteResourceReport
    {
        std::size_t catalogEntries{ 0 };
        std::size_t presentAssets{ 0 };
        std::size_t missingAssets{ 0 };
        std::size_t presentIndexes{ 0 };
        std::size_t missingIndexes{ 0 };
        std::size_t auditedSpritePacks{ 0 };
        std::size_t decodedSmokeFrames{ 0 };
        std::size_t failedSmokeFrames{ 0 };
        std::vector<std::wstring> warnings;
        std::vector<LegacyUiSpriteSmokeFrame> smokeFrames;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return catalogEntries > 0 && presentAssets > 0;
        }
    };

    class LegacyUiSpriteResourceLoader final
    {
    public:
        [[nodiscard]] LegacyUiSpriteResourceReport Load(
            const std::filesystem::path& dataRoot,
            LegacyUiSpriteLoadMode mode = LegacyUiSpriteLoadMode::FullAudit) const;
    };
}
