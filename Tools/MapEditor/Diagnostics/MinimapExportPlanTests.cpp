#include "MapEditorExportModel.h"

#include <iostream>

int main()
{
    const phoenix::tools::map_editor::MapCanvasBounds bounds{ -120, -60, 920, 460 };
    const auto legacy = phoenix::tools::map_editor::BuildGameMinimapScreenshotPlan(bounds);
    const auto legacyFit = phoenix::tools::map_editor::CalculateGameMinimapFit(bounds);
    const auto highQuality = phoenix::tools::map_editor::BuildGameMinimapScreenshotPlan(bounds, 800U, 400U);
    const auto highQualityFit = phoenix::tools::map_editor::CalculateGameMinimapFit(bounds, 800U, 400U);
    const auto firstTile = phoenix::tools::map_editor::CalculateScreenshotSpriteRect(
        0, 0, 48U, 24U, 0.26F, 7, 9);
    const auto adjacentTile = phoenix::tools::map_editor::CalculateScreenshotSpriteRect(
        48, 0, 48U, 24U, 0.26F, 7, 9);

    const bool valid = legacy.width == 200U && legacy.height == 100U &&
        legacy.peakTemporaryBytes == 200U * 100U * 4U &&
        legacyFit.outputWidth == 200U && legacyFit.outputHeight == 100U &&
        legacyFit.contentWidth <= legacyFit.outputWidth && legacyFit.contentHeight <= legacyFit.outputHeight &&
        highQuality.width == 800U && highQuality.height == 400U &&
        highQuality.peakTemporaryBytes == 800U * 400U * 4U &&
        highQualityFit.outputWidth == 800U && highQualityFit.outputHeight == 400U &&
        highQualityFit.contentWidth <= highQualityFit.outputWidth &&
        highQualityFit.contentHeight <= highQualityFit.outputHeight &&
        firstTile.x + firstTile.width == adjacentTile.x;

    std::cout << (valid ? "PASS" : "FAIL")
        << " legacy=" << legacy.width << 'x' << legacy.height
        << " high_quality=" << highQuality.width << 'x' << highQuality.height << '\n';
    return valid ? 0 : 1;
}
