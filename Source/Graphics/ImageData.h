#pragma once

#include <cstdint>
#include <vector>

namespace phoenix::graphics
{
    enum class PixelFormat
    {
        Unknown,
        Bgra8,
        Rgba8,
        LegacyRgb555,
        LegacyRgb565,
        LegacyIndexed8,
        LegacyAlphaIndexed8
    };

    struct ImageData
    {
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        PixelFormat format{ PixelFormat::Unknown };
        std::vector<std::uint8_t> pixels;

        [[nodiscard]] bool Empty() const noexcept
        {
            return width == 0 || height == 0 || pixels.empty();
        }
    };
}
