#pragma once

#include "Assets/LegacyAssetTypes.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace phoenix::assets
{
    class ILegacyIndexReader
    {
    public:
        virtual ~ILegacyIndexReader() = default;

        [[nodiscard]] virtual LegacyReadResult<LegacyIndexTable> ReadIndex(
            const std::filesystem::path& indexPath,
            LegacyAssetFormat companionFormat) const = 0;
    };

    class ILegacySpriteArchiveReader
    {
    public:
        virtual ~ILegacySpriteArchiveReader() = default;

        [[nodiscard]] virtual LegacyReadResult<LegacySpriteArchiveInfo> ReadArchiveInfo(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            LegacyAssetFormat format) const = 0;
    };

    class ILegacyFramePackReader
    {
    public:
        virtual ~ILegacyFramePackReader() = default;

        [[nodiscard]] virtual LegacyReadResult<std::vector<LegacyFrameSequence>> ReadCreatureFrames(
            const std::filesystem::path& framePackPath,
            const std::filesystem::path& frameIndexPath) const = 0;
    };

    class ILegacyAudioTableReader
    {
    public:
        virtual ~ILegacyAudioTableReader() = default;

        [[nodiscard]] virtual LegacyReadResult<std::vector<LegacySoundRef>> ReadSoundTable(
            const std::filesystem::path& soundInfoPath) const = 0;

        [[nodiscard]] virtual LegacyReadResult<std::vector<LegacyMusicRef>> ReadMusicTable(
            const std::filesystem::path& musicInfoPath) const = 0;
    };
}
