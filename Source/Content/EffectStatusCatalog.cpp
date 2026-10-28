#include "Content/EffectStatusCatalog.h"

#include "Content/LegacyBinaryTableReader.h"

namespace phoenix::content
{
    const EffectStatusInfo* EffectStatusCatalogResult::Find(const std::uint16_t effectStatusId) const noexcept
    {
        return effectStatusId < statuses.size() ? &statuses[effectStatusId] : nullptr;
    }

    EffectStatusCatalogResult EffectStatusCatalog::Load(const std::filesystem::path& path) const
    {
        EffectStatusCatalogResult result;
        const auto bytes = ReadBinaryFile(path);
        if (!bytes)
        {
            result.error = L"EffectStatus.inf could not be opened.";
            return result;
        }

        LegacyBinaryTableReader reader(*bytes);
        std::int32_t count = 0;
        if (!reader.I32(count) || count < 0 || count > 100000)
        {
            result.error = L"EffectStatus.inf has invalid table size.";
            return result;
        }

        result.statuses.reserve(static_cast<std::size_t>(count));
        for (std::int32_t i = 0; i < count; ++i)
        {
            EffectStatusInfo status;
            status.effectStatusId = static_cast<std::uint16_t>(i);
            std::uint8_t useEffectSprite = 0;
            std::uint8_t attachGround = 0;
            std::uint8_t colorPart = 0;
            std::uint32_t soundId = 0;
            if (!reader.U8(useEffectSprite) ||
                !reader.U8(attachGround) ||
                !reader.U8(status.new201) ||
                !reader.U16(status.effectSpriteType) ||
                !reader.U16(status.effectColor) ||
                !reader.U8(colorPart) ||
                !reader.U16(status.actionInfo) ||
                !reader.U16(status.originalActionInfo) ||
                !reader.U32(soundId))
            {
                result.error = L"EffectStatus.inf record ended early.";
                result.statuses.clear();
                return result;
            }
            status.useEffectSprite = useEffectSprite != 0;
            status.attachGround = attachGround != 0;
            status.effectColorPart = colorPart;
            status.soundId = static_cast<std::int32_t>(soundId);
            result.statuses.push_back(status);
        }

        return result;
    }
}
