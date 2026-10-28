#include "UI/Resources/LegacyWindowTransparency.h"

#include <algorithm>

namespace phoenix::ui
{
    void LegacyWindowTransparency::SetEnabled(const bool enabled) noexcept
    {
        enabled_ = supported_ && enabled;
    }

    void LegacyWindowTransparency::SetAlphaDepth(const std::uint32_t alphaDepth) noexcept
    {
        alphaDepth_ = static_cast<std::uint8_t>(std::min<std::uint32_t>(alphaDepth, MaximumAlphaDepth));
    }

    void LegacyWindowTransparency::SetAlphaDepthFromTrack(const std::int32_t position, const std::int32_t width) noexcept
    {
        if (width <= 0)
        {
            alphaDepth_ = 0;
            return;
        }

        const auto clampedPosition = std::clamp(position, 0, width);
        alphaDepth_ = static_cast<std::uint8_t>(
            (static_cast<std::int64_t>(clampedPosition) * MaximumAlphaDepth) / width);
    }

    float LegacyWindowTransparency::BackgroundOpacity() const noexcept
    {
        return IsEnabled()
            ? 1.0F - (static_cast<float>(alphaDepth_) / static_cast<float>(MaximumAlphaDepth))
            : 1.0F;
    }

    float LegacyWindowTransparency::TrackRatio() const noexcept
    {
        return static_cast<float>(alphaDepth_) / static_cast<float>(MaximumAlphaDepth);
    }
}
