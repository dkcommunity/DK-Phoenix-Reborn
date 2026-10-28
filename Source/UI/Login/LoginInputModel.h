#pragma once

#include "UI/Layout/LoginTitleLayout.h"

#include <cstddef>
#include <string>

namespace phoenix::ui
{
    enum class LoginInputField
    {
        None,
        Username,
        Password
    };

    class LoginInputModel final
    {
    public:
        void SetLayout(LoginTitleLayout layout);
        [[nodiscard]] const LoginTitleLayout& Layout() const noexcept;

        [[nodiscard]] LoginInputField HitTest(std::int32_t x, std::int32_t y) const noexcept;
        [[nodiscard]] bool Click(std::int32_t x, std::int32_t y) noexcept;
        void Focus(LoginInputField field) noexcept;
        void TabFocus() noexcept;
        void Backspace();
        bool AddChar(char ch);

        [[nodiscard]] LoginInputField FocusedField() const noexcept;
        [[nodiscard]] const std::string& Username() const noexcept;
        [[nodiscard]] const std::string& Password() const noexcept;
        [[nodiscard]] bool ReadyToSubmit() const noexcept;

    private:
        [[nodiscard]] static bool Contains(const UiRect& rect, std::int32_t x, std::int32_t y) noexcept;
        [[nodiscard]] std::string& FocusedText();
        [[nodiscard]] const std::string& FocusedText() const;

        LoginTitleLayout layout_{ LoginTitleLayoutCatalog::Create(LoginTitleResolution::R1280x960) };
        LoginInputField focusedField_{ LoginInputField::Username };
        std::string username_;
        std::string password_;
        std::size_t usernameLimit_{ 12 };
        std::size_t passwordLimit_{ 10 };
    };
}
