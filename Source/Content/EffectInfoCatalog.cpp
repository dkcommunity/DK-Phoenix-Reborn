#include "Content/EffectInfoCatalog.h"

#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace phoenix::content
{
    namespace
    {
        class EffectInfoReader final
        {
        public:
            explicit EffectInfoReader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes))
            {
            }

            [[nodiscard]] bool U8(std::uint8_t& value) noexcept
            {
                if (offset_ + 1U > bytes_.size())
                {
                    return false;
                }
                value = bytes_[offset_++];
                return true;
            }

            [[nodiscard]] bool U16(std::uint16_t& value) noexcept
            {
                if (offset_ + 2U > bytes_.size())
                {
                    return false;
                }
                value = static_cast<std::uint16_t>(bytes_[offset_]) |
                    static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[offset_ + 1U]) << 8U);
                offset_ += 2U;
                return true;
            }

            [[nodiscard]] bool I32(std::int32_t& value) noexcept
            {
                if (offset_ + 4U > bytes_.size())
                {
                    return false;
                }
                const auto raw = static_cast<std::uint32_t>(bytes_[offset_]) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 1U]) << 8U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 2U]) << 16U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 3U]) << 24U);
                offset_ += 4U;
                value = static_cast<std::int32_t>(raw);
                return true;
            }

            [[nodiscard]] bool Skip(const std::size_t size) noexcept
            {
                if (offset_ + size > bytes_.size())
                {
                    return false;
                }
                offset_ += size;
                return true;
            }

            [[nodiscard]] std::size_t Offset() const noexcept
            {
                return offset_;
            }

        private:
            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };
    }

    const EffectSpriteInfo* EffectInfoCatalogResult::Find(const std::uint16_t effectId) const noexcept
    {
        return effectId < effects.size() ? &effects[effectId] : nullptr;
    }

    EffectInfoCatalogResult EffectInfoCatalog::Load(const std::filesystem::path& path) const
    {
        EffectInfoCatalogResult result;
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            result.error = L"EffectSpriteType.inf could not be opened.";
            return result;
        }

        std::vector<std::uint8_t> bytes(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        if (bytes.empty())
        {
            result.error = L"EffectSpriteType.inf is empty.";
            return result;
        }

        EffectInfoReader reader(std::move(bytes));
        std::int32_t effectCount = 0;
        if (!reader.I32(effectCount) || effectCount <= 0 || effectCount > 100000)
        {
            result.error = L"EffectSpriteType.inf has invalid table size.";
            return result;
        }

        result.effects.reserve(static_cast<std::size_t>(effectCount));
        for (std::int32_t effectId = 0; effectId < effectCount; ++effectId)
        {
            std::uint8_t bltType = 0;
            std::uint16_t frameId = 0;
            std::uint8_t flags = 0;
            std::uint16_t actionEffectFrameId = 0;
            std::uint16_t femaleEffectSpriteType = 0;
            std::uint8_t pairCount = 0;
            if (!reader.U8(bltType) ||
                !reader.U16(frameId) ||
                !reader.U8(flags) ||
                !reader.U16(actionEffectFrameId) ||
                !reader.U16(femaleEffectSpriteType) ||
                !reader.U8(pairCount))
            {
                result.error = L"EffectSpriteType.inf record ended early.";
                result.effects.clear();
                return result;
            }

            EffectSpriteInfo info;
            info.effectId = static_cast<std::uint16_t>(effectId);
            info.bltType = bltType <= static_cast<std::uint8_t>(EffectBltType::Screen)
                ? static_cast<EffectBltType>(bltType)
                : EffectBltType::Effect;
            info.frameId = frameId;
            info.repeatFrame = ((flags >> 1U) & 0x01U) != 0U;
            info.pairFrameBack = (flags & 0x01U) != 0U;
            info.actionEffectFrameId = actionEffectFrameId;
            info.femaleEffectSpriteType = femaleEffectSpriteType;
            info.pairFrameIds.reserve(pairCount);

            for (std::uint8_t pairIndex = 0; pairIndex < pairCount; ++pairIndex)
            {
                std::uint16_t pairFrameId = 0;
                if (!reader.U16(pairFrameId))
                {
                    result.error = L"EffectSpriteType.inf pair frame list ended early.";
                    result.effects.clear();
                    return result;
                }
                info.pairFrameIds.push_back(pairFrameId);
            }

            std::uint8_t colorGroupCount = 0;
            if (!reader.U8(colorGroupCount))
            {
                result.error = L"EffectSpriteType.inf color group count ended early.";
                result.effects.clear();
                return result;
            }

            // Legacy stores Color_t + EffectSpriteType for alternate color variants.
            if (!reader.Skip(static_cast<std::size_t>(colorGroupCount) * 4U))
            {
                result.error = L"EffectSpriteType.inf color group list ended early.";
                result.effects.clear();
                return result;
            }

            result.effects.push_back(std::move(info));
        }

        return result;
    }
}
