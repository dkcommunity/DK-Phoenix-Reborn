#pragma once

#include "Assets/SpriteArchiveReader.h"

#include <filesystem>

namespace phoenix::tools::map_editor
{
    class EditorSpriteArchiveReader final
    {
    public:
        [[nodiscard]] assets::LegacyReadResult<assets::LegacyIndexTable> ReadIndex(
            const std::filesystem::path& indexPath,
            const assets::LegacyAssetFormat companionFormat) const
        {
            return reader_.ReadIndex(indexPath, companionFormat);
        }

        [[nodiscard]] assets::LegacyReadResult<assets::LegacySpriteArchiveInfo> ReadArchiveInfo(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            const assets::LegacyAssetFormat format) const
        {
            return reader_.ReadArchiveInfo(archivePath, indexPath, format);
        }

        [[nodiscard]] assets::LegacyReadResult<assets::LegacyDecodedSprite> ReadSprite565(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            const std::uint32_t spriteIndex,
            const assets::LegacyAssetFormat format) const
        {
            if (std::filesystem::is_directory(archivePath) &&
                (format == assets::LegacyAssetFormat::SpritePack ||
                 format == assets::LegacyAssetFormat::ShadowSpritePack))
            {
                // Split chunks are self-indexed. Some valid Legacy archives have stale combined
                // offsets, so Editor decoding must use the chunk's U16 count and record order.
                return reader_.ReadSprite565(
                    archivePath, archivePath / L".editor-chunk-local-index", spriteIndex, format);
            }
            return reader_.ReadSprite565(archivePath, indexPath, spriteIndex, format);
        }

        [[nodiscard]] assets::LegacyReadResult<assets::LegacyDecodedSprite> ReadPalettedSprite565(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& combinedIndexPath,
            const std::uint16_t spriteIndex,
            const std::filesystem::path& palettePackPath,
            const std::uint16_t paletteIndex,
            const bool alphaPixels) const
        {
            return reader_.ReadPalettedSprite565(
                archivePath, combinedIndexPath, spriteIndex, palettePackPath, paletteIndex, alphaPixels);
        }

    private:
        mutable assets::SpriteArchiveReader reader_;
    };
}
