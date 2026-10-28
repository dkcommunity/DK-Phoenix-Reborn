#pragma once

#include <cstdint>
#include <string>

namespace phoenix::media
{
    enum class MediaKind
    {
        StaticImage,
        Video,
        AudioStream
    };

    enum class MediaBackend
    {
        WindowsMediaFoundation
    };

    enum class AspectMode
    {
        Fit,
        Fill,
        Stretch
    };

    struct MediaViewport
    {
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        AspectMode aspectMode{ AspectMode::Fit };
    };

    struct MediaRequest
    {
        std::wstring path;
        MediaKind kind{ MediaKind::StaticImage };
        bool loop{ false };
    };
}
