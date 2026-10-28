#pragma once

#include "Assets/LegacyAssetTypes.h"

#include <filesystem>
#include <vector>

namespace phoenix::assets
{
    class EffectFramePackReader final
    {
    public:
        [[nodiscard]] LegacyReadResult<std::vector<LegacyEffectFrameSequence>> ReadEffectFrames(
            const std::filesystem::path& framePackPath) const;
    };
}
