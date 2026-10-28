#include "Assets/AnimationObjectFramePackReader.h"

#include "Core/BinaryReader.h"

#include <fstream>
#include <iterator>
#include <utility>

namespace phoenix::assets
{
    namespace
    {
        LegacyReadIssue MakeIssue(const std::filesystem::path& path, const std::uint64_t offset, std::wstring message)
        {
            return { LegacyReadSeverity::Error, LegacyAssetFormat::FramePack, path, offset, std::move(message) };
        }

        std::vector<std::byte> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file) return {};
            const std::vector<char> chars{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
            std::vector<std::byte> bytes;
            bytes.reserve(chars.size());
            for (const auto value : chars) bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
            return bytes;
        }
    }

    LegacyReadResult<std::vector<LegacyAnimationObjectFrameSequence>> AnimationObjectFramePackReader::Read(
        const std::filesystem::path& framePackPath) const
    {
        LegacyReadResult<std::vector<LegacyAnimationObjectFrameSequence>> result;
        auto bytes = ReadAllBytes(framePackPath);
        if (bytes.empty())
        {
            result.issues.push_back(MakeIssue(framePackPath, 0, L"Animation object frame pack could not be opened or is empty."));
            return result;
        }

        core::BinaryReader reader(bytes.data(), bytes.size());
        const auto sequenceCount = reader.ReadU16LE();
        if (!sequenceCount)
        {
            result.issues.push_back(MakeIssue(framePackPath, reader.Position(), L"Animation object frame pack is missing its sequence count."));
            return result;
        }

        std::vector<LegacyAnimationObjectFrameSequence> sequences;
        sequences.reserve(*sequenceCount);
        for (std::uint16_t frameId = 0; frameId < *sequenceCount; ++frameId)
        {
            const auto frameCount = reader.ReadU16LE();
            if (!frameCount)
            {
                result.issues.push_back(MakeIssue(framePackPath, reader.Position(), L"Animation object frame pack ended before a frame count."));
                return result;
            }

            LegacyAnimationObjectFrameSequence sequence;
            sequence.frameId = frameId;
            sequence.frames.reserve(*frameCount);
            for (std::uint16_t frameIndex = 0; frameIndex < *frameCount; ++frameIndex)
            {
                const auto spriteId = reader.ReadU16LE();
                const auto cX = reader.ReadU16LE();
                const auto cY = reader.ReadU16LE();
                if (!spriteId || !cX || !cY)
                {
                    result.issues.push_back(MakeIssue(framePackPath, reader.Position(), L"Animation object frame record ended early."));
                    return result;
                }
                sequence.frames.push_back({
                    core::SpriteId(*spriteId),
                    static_cast<std::int16_t>(*cX),
                    static_cast<std::int16_t>(*cY) });
            }
            sequences.push_back(std::move(sequence));
        }

        if (reader.Remaining() != 0U)
        {
            result.issues.push_back(MakeIssue(framePackPath, reader.Position(), L"Animation object frame pack contains trailing data."));
            return result;
        }
        result.value = std::move(sequences);
        return result;
    }
}
