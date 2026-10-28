#pragma once

#include <cstdint>
#include <filesystem>

namespace phoenix::assets
{
    struct ImageArchiveProbeResult
    {
        std::filesystem::path framePackPath;
        std::filesystem::path frameIndexPath;
        std::filesystem::path spriteRootPath;
        std::filesystem::path spriteIndexPath;
        std::filesystem::path shadowRootPath;
        std::filesystem::path shadowIndexPath;
        std::uint64_t framePackBytes{ 0 };
        std::uint16_t frameIndexCount{ 0 };
        bool frameIndexValid{ false };
        bool hasSpritePayload{ false };
        bool hasShadowPayload{ false };
    };

    class ImageArchiveProbe final
    {
    public:
        [[nodiscard]] ImageArchiveProbeResult ProbeCreatureChain(
            const std::filesystem::path& imageRoot,
            const std::wstring& baseName) const;
    };
}
