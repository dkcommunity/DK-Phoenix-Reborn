#include "UI/Login/LoginInputModel.h"

#include <cctype>
#include <utility>

namespace phoenix::ui
{
    void LoginInputModel::SetLayout(LoginTitleLayout layout)
    {
        layout_ = std::move(layout);
    }

    const LoginTitleLayout& LoginInputModel::Layout() const noexcept
    {
        return layout_;
    }

    LoginInputField LoginInputModel::HitTest(const std::int32_t x, const std::int32_t y) const noexcept
    {
        if (Contains(layout_.idEdit, x, y))
        {
            return LoginInputField::Username;
        }

        if (Contains(layout_.passwordEdit, x, y))
        {
            return LoginInputField::Password;
        }

        return LoginInputField::None;
    }

    bool LoginInputModel::Click(const std::int32_t x, const std::int32_t y) noexcept
    {
        const auto hit = HitTest(x, y);
        if (hit == LoginInputField::None)
        {
            return false;
        }

        focusedField_ = hit;
        return true;
    }

    void LoginInputModel::Focus(const LoginInputField field) noexcept
    {
        focusedField_ = field;
    }

    void LoginInputModel::TabFocus() noexcept
    {
        focusedField_ = focusedField_ == LoginInputField::Username ? LoginInputField::Password : LoginInputField::Username;
    }

    void LoginInputModel::Backspace()
    {
        auto& text = FocusedText();
        if (!text.empty())
        {
            text.pop_back();
        }
    }

    bool LoginInputModel::AddChar(const char ch)
    {
        if (focusedField_ == LoginInputField::None || !std::isprint(static_cast<unsigned char>(ch)) || std::isspace(static_cast<unsigned char>(ch)))
        {
            return false;
        }

        auto& text = FocusedText();
        const auto limit = focusedField_ == LoginInputField::Username ? usernameLimit_ : passwordLimit_;
        if (text.size() >= limit)
        {
            return false;
        }

        text.push_back(ch);
        return true;
    }

    LoginInputField LoginInputModel::FocusedField() const noexcept
    {
        return focusedField_;
    }

    const std::string& LoginInputModel::Username() const noexcept
    {
        return username_;
    }

    const std::string& LoginInputModel::Password() const noexcept
    {
        return password_;
    }

    bool LoginInputModel::ReadyToSubmit() const noexcept
    {
        return !username_.empty() && !password_.empty();
    }

    bool LoginInputModel::Contains(const UiRect& rect, const std::int32_t x, const std::int32_t y) noexcept
    {
        return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
    }

    std::string& LoginInputModel::FocusedText()
    {
        return focusedField_ == LoginInputField::Password ? password_ : username_;
    }

    const std::string& LoginInputModel::FocusedText() const
    {
        return focusedField_ == LoginInputField::Password ? password_ : username_;
    }
}
