#pragma once

#include "UI/Login/LoginInputModel.h"
#include "UI/UiTypes.h"

#include <string>
#include <vector>

namespace phoenix::ui::text
{
    struct LoginTextRun
    {
        UiRect rect;
        std::wstring text;
        bool passwordMasked{ false };
        std::uint32_t color{ 0x00FFFFFFU };
    };

    class LoginTextLayout final
    {
    public:
        [[nodiscard]] static std::vector<LoginTextRun> Build(const ui::LoginInputModel& input);
    };
}
