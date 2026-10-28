#include "UI/Layout/LayoutViewport.h"

#include <algorithm>
#include <cmath>

namespace phoenix::ui
{
    LayoutTransform LayoutViewport::BuildTransform(
        const ReferenceCanvas reference,
        const ScreenViewport viewport) noexcept
    {
        LayoutTransform transform;
        if (reference.width == 0 || reference.height == 0 || viewport.width == 0 || viewport.height == 0)
        {
            return transform;
        }

        transform.scaleX = static_cast<float>(viewport.width) / static_cast<float>(reference.width);
        transform.scaleY = static_cast<float>(viewport.height) / static_cast<float>(reference.height);
        transform.uniformScale = std::min(transform.scaleX, transform.scaleY);
        transform.offsetX = static_cast<std::int32_t>((static_cast<float>(viewport.width) - static_cast<float>(reference.width) * transform.uniformScale) * 0.5F);
        transform.offsetY = static_cast<std::int32_t>((static_cast<float>(viewport.height) - static_cast<float>(reference.height) * transform.uniformScale) * 0.5F);
        return transform;
    }

    UiRect LayoutViewport::ScaleRect(const UiRect rect, const LayoutTransform transform) noexcept
    {
        return UiRect{
            transform.offsetX + static_cast<std::int32_t>(std::lround(static_cast<float>(rect.x) * transform.uniformScale)),
            transform.offsetY + static_cast<std::int32_t>(std::lround(static_cast<float>(rect.y) * transform.uniformScale)),
            static_cast<std::int32_t>(std::lround(static_cast<float>(rect.width) * transform.uniformScale)),
            static_cast<std::int32_t>(std::lround(static_cast<float>(rect.height) * transform.uniformScale))
        };
    }

    AspectRect LayoutViewport::ResolveAspectRect(
        const ReferenceCanvas source,
        const ScreenViewport target,
        const AspectRatioPolicy policy) noexcept
    {
        AspectRect result;
        result.policy = policy;
        if (source.width == 0 || source.height == 0 || target.width == 0 || target.height == 0)
        {
            return result;
        }

        if (policy == AspectRatioPolicy::Stretch)
        {
            result.rect = UiRect{ 0, 0, static_cast<std::int32_t>(target.width), static_cast<std::int32_t>(target.height) };
            return result;
        }

        const auto sx = static_cast<float>(target.width) / static_cast<float>(source.width);
        const auto sy = static_cast<float>(target.height) / static_cast<float>(source.height);
        const auto scale = policy == AspectRatioPolicy::Fill ? std::max(sx, sy) : std::min(sx, sy);
        const auto width = static_cast<std::int32_t>(std::lround(static_cast<float>(source.width) * scale));
        const auto height = static_cast<std::int32_t>(std::lround(static_cast<float>(source.height) * scale));
        result.rect = UiRect{
            (static_cast<std::int32_t>(target.width) - width) / 2,
            (static_cast<std::int32_t>(target.height) - height) / 2,
            width,
            height
        };
        return result;
    }
}
