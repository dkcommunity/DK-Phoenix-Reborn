#pragma once

#include "Assets/LegacyAssetTypes.h"

#include <filesystem>
#include <vector>

namespace phoenix::assets
{
    class AnimationObjectFramePackReader final
    {
    public:
        [[nodiscard]] LegacyReadResult<std::vector<LegacyAnimationObjectFrameSequence>> Read(
            const std::filesystem::path& framePackPath) const;
    };
}
