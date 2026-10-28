#include "MapEditorViewportModel.h"

#include <algorithm>
#include <cmath>

namespace phoenix::tools::map_editor
{
    MapCanvasBounds CalculateMapCanvasBounds(const world::LegacyMap& map) noexcept
    {
        return MapCanvasBounds{
            0,
            0,
            static_cast<std::int64_t>(map.width) * 48,
            static_cast<std::int64_t>(map.height) * 24
        };
    }

    float CalculateFitZoom(const MapCanvasBounds& bounds, const std::int32_t viewportWidth,
        const std::int32_t viewportHeight, const std::int32_t padding) noexcept
    {
        if (bounds.Width() <= 0 || bounds.Height() <= 0 || viewportWidth <= 0 || viewportHeight <= 0)
            return 1.0F;
        const auto availableWidth = std::max(1, viewportWidth - std::max(0, padding) * 2);
        const auto availableHeight = std::max(1, viewportHeight - std::max(0, padding) * 2);
        const auto horizontal = static_cast<float>(availableWidth) / static_cast<float>(bounds.Width());
        const auto vertical = static_cast<float>(availableHeight) / static_cast<float>(bounds.Height());
        return std::clamp(std::min(horizontal, vertical), 0.02F, 1.0F);
    }

    std::int32_t CalculateVisibleTileRadius(const std::int32_t viewportWidth,
        const std::int32_t viewportHeight, const float zoom) noexcept
    {
        const auto scale = std::max(0.02F, zoom);
        const auto horizontal = static_cast<std::int32_t>(std::ceil(
            static_cast<float>(std::max(0, viewportWidth)) / (2.0F * 48.0F * scale)));
        const auto vertical = static_cast<std::int32_t>(std::ceil(
            static_cast<float>(std::max(0, viewportHeight)) / (2.0F * 24.0F * scale)));
        return std::max(horizontal, vertical) + 2;
    }

    ScaledSpriteRect CalculateScaledSpriteRect(const std::int32_t x, const std::int32_t y,
        const std::uint32_t width, const std::uint32_t height,
        const float centerX, const float centerY, const float zoom) noexcept
    {
        const auto scale = std::max(0.02F, zoom);
        const auto scaleX = [&](const std::int64_t value) {
            return static_cast<std::int32_t>(std::lround(
                centerX + (static_cast<float>(value) - centerX) * scale));
        };
        const auto scaleY = [&](const std::int64_t value) {
            return static_cast<std::int32_t>(std::lround(
                centerY + (static_cast<float>(value) - centerY) * scale));
        };
        const auto left = scaleX(x);
        const auto top = scaleY(y);
        const auto right = scaleX(static_cast<std::int64_t>(x) + width);
        const auto bottom = scaleY(static_cast<std::int64_t>(y) + height);
        return { left, top, std::max(1, right - left), std::max(1, bottom - top) };
    }
}
