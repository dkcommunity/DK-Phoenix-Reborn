#pragma once

#include "UI/UiTypes.h"

#include <vector>

namespace phoenix::ui
{
    enum class UiElementState
    {
        Normal,
        Hovered,
        Pressed,
        Disabled,
        Hidden
    };

    struct UiElement
    {
        UiElementId id;
        UiRect rect;
        UiAnchor anchor{ UiAnchor::TopLeft };
        UiElementState state{ UiElementState::Normal };
        std::vector<UiResourceRef> resources;
        std::vector<UiElement> children;
    };
}
