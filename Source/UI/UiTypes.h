#pragma once

#include <cstdint>
#include <string>

namespace phoenix::ui
{
    enum class UiAnchor
    {
        TopLeft,
        Top,
        TopRight,
        Left,
        Center,
        Right,
        BottomLeft,
        Bottom,
        BottomRight
    };

    enum class UiResourceKind
    {
        Image,
        Font,
        Cursor,
        Layout,
        Sound
    };

    struct UiRect
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::int32_t width{ 0 };
        std::int32_t height{ 0 };
    };

    struct UiElementId
    {
        std::wstring value;
    };

    struct UiResourceRef
    {
        UiResourceKind kind{ UiResourceKind::Image };
        std::wstring logicalName;
        std::wstring legacyPath;
    };
}
