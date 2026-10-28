#pragma once

#include "Core/StrongId.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace phoenix::ui
{
    enum class TitleMenuButtonRole
    {
        Login,
        Option,
        Credits,
        Exit,
        NewAccount
    };

    struct TitleMenuButtonFrames
    {
        TitleMenuButtonRole role{ TitleMenuButtonRole::Login };
        core::SpriteId highlightedFrame;
        core::SpriteId normalFrame;
        std::wstring_view legacyName;
    };

    class TitleMenuButtonMap final
    {
    public:
        [[nodiscard]] static std::vector<TitleMenuButtonFrames> CreateDefault();
        [[nodiscard]] static const TitleMenuButtonFrames* Find(
            const std::vector<TitleMenuButtonFrames>& map,
            TitleMenuButtonRole role) noexcept;
    };
}
