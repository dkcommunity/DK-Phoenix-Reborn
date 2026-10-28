#include "Assets/FramePackReader.h"

#include "Assets/SpriteArchiveReader.h"
#include "Core/BinaryReader.h"

#include <fstream>
#include <iterator>

namespace phoenix::assets
{
    namespace
    {
        LegacyReadIssue MakeFrameIssue(const std::filesystem::path& path, std::uint64_t offset, std::wstring message)
        {
            return LegacyReadIssue{ LegacyReadSeverity::Error, LegacyAssetFormat::FramePack, path, offset, std::move(message) };
        }

        LegacyReadIssue MakeFrameWarning(const std::filesystem::path& path, std::uint64_t offset, std::wstring message)
        {
            return LegacyReadIssue{ LegacyReadSeverity::Warning, LegacyAssetFormat::FramePack, path, offset, std::move(message) };
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

    LegacyReadResult<std::vector<LegacyFrameSequence>> FramePackReader::ReadCreatureFrames(
        const std::filesystem::path& framePackPath,
        const std::filesystem::path& frameIndexPath) const
    {
        LegacyReadResult<std::vector<LegacyFrameSequence>> result;
        auto bytes = ReadAllBytes(framePackPath);
        if (bytes.empty())
        {
            result.issues.push_back(MakeFrameIssue(framePackPath, 0, L"Frame pack could not be opened or is empty."));
            return result;
        }

        SpriteArchiveReader indexReader;
        auto index = indexReader.ReadIndex(frameIndexPath, LegacyAssetFormat::FramePack);
        if (!index.value)
        {
            result.issues.insert(result.issues.end(), index.issues.begin(), index.issues.end());
        }

        core::BinaryReader reader(bytes.data(), bytes.size());
        auto bodyCount = reader.ReadU16LE();
        if (!bodyCount)
        {
            result.issues.push_back(MakeFrameIssue(framePackPath, reader.Position(), L"Frame pack missing body count."));
            return result;
        }
        if (*bodyCount > 4096U)
        {
            result.issues.push_back(MakeFrameIssue(framePackPath, 0U, L"Frame pack body count exceeds the Legacy safety limit."));
            return result;
        }

        std::vector<LegacyFrameSequence> sequences;
        for (std::uint16_t body = 0; body < *bodyCount; ++body)
        {
            auto actionCount = reader.ReadU8();
            if (!actionCount)
            {
                result.issues.push_back(MakeFrameIssue(framePackPath, reader.Position(), L"Frame pack ended before action count."));
                return result;
            }
            if (*actionCount > 64U)
            {
                result.issues.push_back(MakeFrameIssue(
                    framePackPath,
                    reader.Position() - 1U,
                    L"Frame pack action count exceeds the Legacy safety limit at body=" + std::to_wstring(body) + L"."));
                return result;
            }

            for (std::uint8_t action = 0; action < *actionCount; ++action)
            {
                auto directionCount = reader.ReadU8();
                if (!directionCount)
                {
                    result.issues.push_back(MakeFrameIssue(framePackPath, reader.Position(), L"Frame pack ended before direction count."));
                    return result;
                }
                if (*directionCount != 0U && *directionCount != 8U)
                {
                    result.issues.push_back(MakeFrameIssue(
                        framePackPath,
                        reader.Position() - 1U,
                        L"Frame pack direction count is not 8 at body=" + std::to_wstring(body) +
                            L" action=" + std::to_wstring(action) +
                            L" count=" + std::to_wstring(*directionCount) + L"."));
                }

                for (std::uint8_t direction = 0; direction < *directionCount; ++direction)
                {
                    auto frameCount = reader.ReadU16LE();
                    if (!frameCount)
                    {
                        result.issues.push_back(MakeFrameIssue(framePackPath, reader.Position(), L"Frame pack ended before frame count."));
                        return result;
                    }
                    const auto frameBytes = static_cast<std::size_t>(*frameCount) * 6U;
                    if (frameBytes > reader.Remaining())
                    {
                        result.issues.push_back(MakeFrameIssue(
                            framePackPath,
                            reader.Position(),
                            L"Frame count exceeds remaining bytes at body=" + std::to_wstring(body) +
                                L" action=" + std::to_wstring(action) +
                                L" direction=" + std::to_wstring(direction) + L"."));
                        return result;
                    }

                    LegacyFrameSequence sequence;
                    sequence.bodyId = core::BodyId(body);
                    sequence.actionId = core::ActionId(action);
                    sequence.directionId = core::DirectionId(direction);
                    sequence.frames.reserve(*frameCount);

                    for (std::uint16_t frame = 0; frame < *frameCount; ++frame)
                    {
                        auto sprite = reader.ReadU16LE();
                        auto originX = reader.ReadU16LE();
                        auto originY = reader.ReadU16LE();
                        if (!sprite || !originX || !originY)
                        {
                            result.issues.push_back(MakeFrameIssue(framePackPath, reader.Position(), L"Frame sequence ended before all frame refs were read."));
                            return result;
                        }

                        sequence.frames.push_back(LegacyFrameRef{
                            core::SpriteId(*sprite),
                            static_cast<std::int16_t>(*originX),
                            static_cast<std::int16_t>(*originY)
                        });
                    }

                    sequences.push_back(std::move(sequence));
                }
            }
        }

        if (reader.Remaining() != 0U)
        {
            result.issues.push_back(MakeFrameWarning(
                framePackPath,
                reader.Position(),
                L"Frame pack has " + std::to_wstring(reader.Remaining()) + L" trailing bytes."));
        }

        result.value = std::move(sequences);
        return result;
    }
}
