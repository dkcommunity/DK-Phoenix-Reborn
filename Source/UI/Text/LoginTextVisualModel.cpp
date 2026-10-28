#include "UI/Text/LoginTextVisualModel.h"

namespace phoenix::ui::text
{
    LoginTextVisualState LoginTextVisualModel::Build(const ui::LoginInputModel& input) noexcept
    {
        const auto& layout = input.Layout();
        LoginTextVisualState state;
        const auto focusedField = input.FocusedField();
        state.passwordFocused = focusedField == ui::LoginInputField::Password;
        state.usernameFocused = focusedField == ui::LoginInputField::Username;
        state.focusRect = state.passwordFocused ? layout.passwordEdit : layout.idEdit;

        const auto& focusedText = state.passwordFocused ? input.Password() : input.Username();
        const auto caretX = state.focusRect.x + 4 + static_cast<std::int32_t>(focusedText.size()) * 8;
        state.caretRect = UiRect{
            caretX,
            state.focusRect.y + 3,
            1,
            state.focusRect.height - 6
        };
        return state;
    }
}
