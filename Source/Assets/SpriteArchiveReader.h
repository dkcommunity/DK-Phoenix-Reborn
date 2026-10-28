#pragma once

#include "Assets/LegacyReaderContracts.h"
#include "Graphics/ImageData.h"

#include <array>
#include <optional>

namespace phoenix::assets
{
    struct LegacyIndexedColorMapping
    {
        std::array<std::uint16_t, 256> colorSetByIndex{};
    };

    struct LegacySpriteMetadata
    {
        std::uint32_t spriteIndex{ 0 };
        std::uint32_t offset{ 0 };
        std::uint32_t nextOffset{ 0 };
        std::uint32_t byteSize{ 0 };
        std::uint16_t width{ 0 };
        std::uint16_t height{ 0 };
        bool valid{ false };
    };

    struct LegacyDecodedSprite
    {
        LegacySpriteMetadata metadata;
        graphics::ImageData image;
    };

    struct LegacySpriteAtlasReport
    {
        std::filesystem::path archivePath;
        std::filesystem::path indexPath;
        std::uint16_t spriteCount{ 0 };
        std::vector<LegacySpriteMetadata> sprites;
    };

    struct LegacySpriteReadMetrics
    {
        std::uint64_t atlasNanoseconds{ 0 };
        std::uint64_t archiveAcquireNanoseconds{ 0 };
        std::uint64_t decodeNanoseconds{ 0 };
        std::uint32_t archiveCacheHits{ 0 };
        std::uint32_t archiveCacheMisses{ 0 };
        std::uint32_t combinedIndexCacheHits{ 0 };
        std::uint32_t combinedIndexBuilds{ 0 };
    };

    class SpriteArchiveReader final : public ILegacyIndexReader, public ILegacySpriteArchiveReader
    {
    public:
        void ResetMetrics() noexcept;
        [[nodiscard]] LegacySpriteReadMetrics Metrics() const noexcept;
        [[nodiscard]] LegacyReadResult<LegacyIndexTable> ReadIndex(
            const std::filesystem::path& indexPath,
            LegacyAssetFormat companionFormat) const override;

        [[nodiscard]] LegacyReadResult<LegacySpriteArchiveInfo> ReadArchiveInfo(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            LegacyAssetFormat format) const override;

        [[nodiscard]] LegacyReadResult<LegacySpriteAtlasReport> InspectSpriteAtlas(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            LegacyAssetFormat format) const;

        [[nodiscard]] LegacyReadResult<LegacyDecodedSprite> ReadSprite565(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            std::uint32_t spriteIndex,
            LegacyAssetFormat format) const;

        [[nodiscard]] LegacyReadResult<LegacyDecodedSprite> ReadPalettedSprite565(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& combinedIndexPath,
            std::uint16_t spriteIndex,
            const std::filesystem::path& palettePackPath,
            std::uint16_t paletteIndex,
            bool alphaPixels) const;

        [[nodiscard]] LegacyReadResult<LegacyDecodedSprite> ReadSplitSprite565(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& combinedIndexPath,
            std::uint32_t combinedSpriteIndex,
            std::uint32_t splitBaseSpriteIndex,
            LegacyAssetFormat format) const;

        [[nodiscard]] LegacyReadResult<LegacyDecodedSprite> ReadSplitIndexedSprite565(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& combinedIndexPath,
            std::uint32_t combinedSpriteIndex,
            std::uint32_t splitBaseSpriteIndex,
            LegacyAssetFormat format,
            const LegacyIndexedColorMapping* colorMapping = nullptr) const;

    private:
        mutable LegacySpriteReadMetrics metrics_;
    };
}
