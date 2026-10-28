#pragma once

#include "UI/UiTypes.h"

#include <cstdint>

namespace phoenix::ui
{
    enum class AspectRatioPolicy
    {
        Fit,
        Fill,
        Stretch
    };

    struct ReferenceCanvas
    {
        std::uint32_t width{ 1280 };
        std::uint32_t height{ 960 };
    };

    struct ScreenViewport
    {
        std::uint32_t width{ 1280 };
        std::uint32_t height{ 960 };
    };

    struct LayoutTransform
    {
        float scaleX{ 1.0F };
        float scaleY{ 1.0F };
        float uniformScale{ 1.0F };
        std::int32_t offsetX{ 0 };
        std::int32_t offsetY{ 0 };
    };

    struct AspectRect
    {
        UiRect rect;
        AspectRatioPolicy policy{ AspectRatioPolicy::Fit };
    };

    class LayoutViewport final
    {
    public:
        [[nodiscard]] static LayoutTransform BuildTransform(
            ReferenceCanvas reference,
            ScreenViewport viewport) noexcept;

        [[nodiscard]] static UiRect ScaleRect(
            UiRect rect,
            LayoutTransform transform) noexcept;

        [[nodiscard]] static AspectRect ResolveAspectRect(
            ReferenceCanvas source,
            ScreenViewport target,
            AspectRatioPolicy policy) noexcept;
    };
}
