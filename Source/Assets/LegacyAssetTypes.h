#pragma once

#include "Core/StrongId.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::assets
{
    enum class LegacyAssetFormat
    {
        Unknown,
        InfTable,
        SpritePack,
        IndexedSpritePack,
        ShadowSpritePack,
        AlphaSpritePack,
        FramePack,
        PalettePack,
        WavePack,
        ResourcePack
    };

    enum class LegacyReadSeverity
    {
        Info,
        Warning,
        Error
    };

    struct LegacyReadIssue
    {
        LegacyReadSeverity severity{ LegacyReadSeverity::Error };
        LegacyAssetFormat format{ LegacyAssetFormat::Unknown };
        std::filesystem::path sourcePath;
        std::uint64_t byteOffset{ 0 };
        std::wstring message;
    };

    template <typename T>
    struct LegacyReadResult
    {
        std::optional<T> value;
        std::vector<LegacyReadIssue> issues;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return value.has_value();
        }
    };

    struct LegacyIndexTable
    {
        std::uint16_t count{ 0 };
        std::vector<std::uint32_t> offsets;
    };

    struct LegacySpriteArchiveInfo
    {
        LegacyAssetFormat format{ LegacyAssetFormat::Unknown };
        std::uint16_t spriteCount{ 0 };
        std::filesystem::path archivePath;
        std::filesystem::path indexPath;
    };

    struct LegacyFrameRef
    {
        core::SpriteId spriteId;
        std::int32_t originX{ 0 };
        std::int32_t originY{ 0 };
    };

    struct LegacyFrameSequence
    {
        core::BodyId bodyId;
        core::ActionId actionId;
        core::DirectionId directionId;
        std::vector<LegacyFrameRef> frames;
    };

    struct LegacyAnimationObjectFrameSequence
    {
        std::uint16_t frameId{ 0 };
        std::vector<LegacyFrameRef> frames;
    };

    struct LegacyEffectFrameRef
    {
        core::SpriteId spriteId;
        std::int32_t originX{ 0 };
        std::int32_t originY{ 0 };
        std::uint8_t light{ 0 };
        bool background{ false };
    };

    struct LegacyEffectFrameSequence
    {
        std::uint16_t frameId{ 0 };
        core::DirectionId directionId;
        std::vector<LegacyEffectFrameRef> frames;
    };

    struct LegacySoundRef
    {
        core::AudioId audioId;
        std::filesystem::path filename;
    };

    struct LegacyMusicRef
    {
        core::AudioId audioId;
        std::filesystem::path musicFilename;
        std::filesystem::path waveFilename;
    };
}
