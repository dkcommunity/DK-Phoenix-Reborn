#pragma once

#include <wincodec.h>

#include <cstdint>
#include <filesystem>
#include <functional>

namespace phoenix::tools::map_editor
{
    using PngPixelWriter = std::function<bool(IWICBitmapFrameEncode*)>;

    [[nodiscard]] bool WritePngImage(const std::filesystem::path& path,
        std::uint32_t width, std::uint32_t height, const PngPixelWriter& writePixels);
}
