#pragma once

#include "MapEditorViewportModel.h"

#include <cstddef>
#include <cstdint>

namespace phoenix::tools::map_editor
{
    struct ScreenshotLayerPolicy
    {
        bool tiles{ true };
        bool objects{ true };
        bool animations{ true };
        bool grid{ false };
        bool ruler{ false };
        bool selection{ false };
        bool hover{ false };
        bool collision{ false };
        bool cameraRectangle{ false };
    };

    struct ScreenshotPlan
    {
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        std::uint32_t stripHeight{ 0 };
        std::uint32_t stripCount{ 0 };
        std::size_t peakTemporaryBytes{ 0 };
        MapCanvasBounds sourceBounds;
        ScreenshotLayerPolicy layers;
    };

    struct MinimapFit
    {
        std::uint32_t outputWidth{ 200 };
        std::uint32_t outputHeight{ 100 };
        std::int32_t contentX{ 0 };
        std::int32_t contentY{ 0 };
        std::uint32_t contentWidth{ 200 };
        std::uint32_t contentHeight{ 100 };
        float scale{ 1.0F };
    };

    [[nodiscard]] ScreenshotPlan BuildFullMapScreenshotPlan(
        const MapCanvasBounds& bounds, std::size_t memoryBudgetBytes = 16U * 1024U * 1024U) noexcept;
    [[nodiscard]] ScreenshotPlan BuildGameMinimapScreenshotPlan(const MapCanvasBounds& bounds,
        std::uint32_t outputWidth = 200U, std::uint32_t outputHeight = 100U) noexcept;
    [[nodiscard]] MinimapFit CalculateGameMinimapFit(const MapCanvasBounds& bounds,
        std::uint32_t outputWidth = 200U, std::uint32_t outputHeight = 100U) noexcept;
    [[nodiscard]] ScaledSpriteRect CalculateScreenshotSpriteRect(
        std::int32_t worldX, std::int32_t worldY, std::uint32_t width, std::uint32_t height,
        float scale, std::int32_t offsetX, std::int32_t offsetY) noexcept;
}
