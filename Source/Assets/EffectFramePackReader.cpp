#include "Assets/EffectFramePackReader.h"

#include "Core/BinaryReader.h"

#include <fstream>
#include <iterator>
#include <utility>

namespace phoenix::assets
{
    namespace
    {
        LegacyReadIssue MakeEffectFrameIssue(
            const std::filesystem::path& path,
            const std::uint64_t offset,
            std::wstring message)
        {
            return LegacyReadIssue{ LegacyReadSeverity::Error, LegacyAssetFormat::FramePack, path, offset, std::move(message) };
        }

        std::vector<std::byte> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            const std::vector<char> chars{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };

            std::vector<std::byte> bytes;
            bytes.reserve(chars.size());
            for (const auto value : chars)
            {
                bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
            }
            return bytes;
        }
    }

    LegacyReadResult<std::vector<LegacyEffectFrameSequence>> EffectFramePackReader::ReadEffectFrames(
        const std::filesystem::path& framePackPath) const
    {
        LegacyReadResult<std::vector<LegacyEffectFrameSequence>> result;
        auto bytes = ReadAllBytes(framePackPath);
        if (bytes.empty())
        {
            result.issues.push_back(MakeEffectFrameIssue(framePackPath, 0, L"Effect frame pack could not be opened or is empty."));
            return result;
        }

        core::BinaryReader reader(bytes.data(), bytes.size());
        const auto frameSetCount = reader.ReadU16LE();
        if (!frameSetCount)
        {
            result.issues.push_back(MakeEffectFrameIssue(framePackPath, reader.Position(), L"Effect frame pack missing frame set count."));
            return result;
        }

        std::vector<LegacyEffectFrameSequence> sequences;
        for (std::uint16_t frameId = 0; frameId < *frameSetCount; ++frameId)
        {
            const auto directionCount = reader.ReadU8();
            if (!directionCount)
            {
                result.issues.push_back(MakeEffectFrameIssue(framePackPath, reader.Position(), L"Effect frame pack ended before direction count."));
                return result;
            }

            for (std::uint8_t direction = 0; direction < *directionCount; ++direction)
            {
                const auto frameCount = reader.ReadU16LE();
                if (!frameCount)
                {
                    result.issues.push_back(MakeEffectFrameIssue(framePackPath, reader.Position(), L"Effect frame pack ended before frame count."));
                    return result;
                }

                LegacyEffectFrameSequence sequence;
                sequence.frameId = frameId;
                sequence.directionId = core::DirectionId(direction);
                sequence.frames.reserve(*frameCount);

                for (std::uint16_t frameIndex = 0; frameIndex < *frameCount; ++frameIndex)
                {
                    const auto sprite = reader.ReadU16LE();
                    const auto originX = reader.ReadU16LE();
                    const auto originY = reader.ReadU16LE();
                    const auto packedLight = reader.ReadU8();
                    if (!sprite || !originX || !originY || !packedLight)
                    {
                        result.issues.push_back(MakeEffectFrameIssue(framePackPath, reader.Position(), L"Effect frame record ended early."));
                        return result;
                    }

                    sequence.frames.push_back(LegacyEffectFrameRef{
                        core::SpriteId(*sprite),
                        static_cast<std::int16_t>(*originX),
                        static_cast<std::int16_t>(*originY),
                        static_cast<std::uint8_t>(*packedLight & 0x7FU),
                        (*packedLight & 0x80U) != 0U
                    });
                }

                sequences.push_back(std::move(sequence));
            }
        }

        result.value = std::move(sequences);
        return result;
    }

}
