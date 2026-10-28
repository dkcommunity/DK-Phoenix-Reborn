#include "MapEditorExportModel.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace phoenix::tools::map_editor
{
    namespace
    {
        std::uint32_t BoundedDimension(const std::int64_t value) noexcept
        {
            return static_cast<std::uint32_t>(std::clamp<std::int64_t>(
                value, 1, std::numeric_limits<std::uint32_t>::max()));
        }
    }

    ScreenshotPlan BuildFullMapScreenshotPlan(const MapCanvasBounds& bounds,
        const std::size_t memoryBudgetBytes) noexcept
    {
        ScreenshotPlan plan;
        plan.sourceBounds = bounds;
        plan.width = BoundedDimension(bounds.Width());
        plan.height = BoundedDimension(bounds.Height());
        const auto rowBytes = std::max<std::size_t>(4U, static_cast<std::size_t>(plan.width) * 4U);
        const auto budgetRows = std::max<std::size_t>(1U, std::max<std::size_t>(rowBytes, memoryBudgetBytes) / rowBytes);
        plan.stripHeight = static_cast<std::uint32_t>(std::min<std::size_t>(
            std::min<std::size_t>(256U, budgetRows), plan.height));
        plan.stripCount = (plan.height + plan.stripHeight - 1U) / plan.stripHeight;
        plan.peakTemporaryBytes = rowBytes * plan.stripHeight;
        return plan;
    }

    MinimapFit CalculateGameMinimapFit(const MapCanvasBounds& bounds,
        const std::uint32_t outputWidth, const std::uint32_t outputHeight) noexcept
    {
        MinimapFit fit;
        fit.outputWidth = std::max(1U, outputWidth);
        fit.outputHeight = std::max(1U, outputHeight);
        const auto sourceWidth = static_cast<double>(std::max<std::int64_t>(1, bounds.Width()));
        const auto sourceHeight = static_cast<double>(std::max<std::int64_t>(1, bounds.Height()));
        const auto scale = std::min(static_cast<double>(fit.outputWidth) / sourceWidth,
            static_cast<double>(fit.outputHeight) / sourceHeight);
        fit.scale = static_cast<float>(scale);
        fit.contentWidth = static_cast<std::uint32_t>(std::clamp<long long>(
            std::llround(sourceWidth * scale), 1LL, static_cast<long long>(fit.outputWidth)));
        fit.contentHeight = static_cast<std::uint32_t>(std::clamp<long long>(
            std::llround(sourceHeight * scale), 1LL, static_cast<long long>(fit.outputHeight)));
        fit.contentX = static_cast<std::int32_t>((fit.outputWidth - fit.contentWidth) / 2U);
        fit.contentY = static_cast<std::int32_t>((fit.outputHeight - fit.contentHeight) / 2U);
        return fit;
    }

    ScreenshotPlan BuildGameMinimapScreenshotPlan(const MapCanvasBounds& bounds,
        const std::uint32_t outputWidth, const std::uint32_t outputHeight) noexcept
    {
        ScreenshotPlan plan;
        plan.sourceBounds = bounds;
        plan.width = std::max(1U, outputWidth);
        plan.height = std::max(1U, outputHeight);
        plan.stripHeight = plan.height;
        plan.stripCount = 1U;
        plan.peakTemporaryBytes = static_cast<std::size_t>(plan.width) * plan.height * 4U;
        return plan;
    }

    ScaledSpriteRect CalculateScreenshotSpriteRect(
        const std::int32_t worldX, const std::int32_t worldY,
        const std::uint32_t width, const std::uint32_t height,
        const float scale, const std::int32_t offsetX, const std::int32_t offsetY) noexcept
    {
        const auto left = offsetX + static_cast<std::int32_t>(std::lround(
            static_cast<double>(worldX) * scale));
        const auto top = offsetY + static_cast<std::int32_t>(std::lround(
            static_cast<double>(worldY) * scale));
        const auto right = offsetX + static_cast<std::int32_t>(std::lround(
            (static_cast<double>(worldX) + width) * scale));
        const auto bottom = offsetY + static_cast<std::int32_t>(std::lround(
            (static_cast<double>(worldY) + height) * scale));
        return { left, top, std::max(1, right - left), std::max(1, bottom - top) };
    }
}
