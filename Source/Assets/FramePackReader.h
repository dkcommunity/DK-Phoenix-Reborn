#pragma once

#include "Assets/LegacyReaderContracts.h"

namespace phoenix::assets
{
    class FramePackReader final : public ILegacyFramePackReader
    {
    public:
        [[nodiscard]] LegacyReadResult<std::vector<LegacyFrameSequence>> ReadCreatureFrames(
            const std::filesystem::path& framePackPath,
            const std::filesystem::path& frameIndexPath) const override;
    };
}
