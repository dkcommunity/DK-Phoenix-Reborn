#pragma once

#include "UI/Login/LoginInputModel.h"
#include "UI/UiTypes.h"

namespace phoenix::ui::text
{
    struct LoginTextVisualState
    {
        UiRect focusRect;
        UiRect caretRect;
        bool passwordFocused{ false };
        bool usernameFocused{ true };
    };

    class LoginTextVisualModel final
    {
    public:
        [[nodiscard]] static LoginTextVisualState Build(const ui::LoginInputModel& input) noexcept;
    };
}
