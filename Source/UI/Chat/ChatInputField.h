#pragma once

#include "UI/UiTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace phoenix::ui
{
    class ChatInputField final
    {
    public:
        static constexpr std::size_t LegacyByteLimit = 100;

        [[nodiscard]] bool IsVisible() const noexcept { return visible_; }
        void Show(bool visible) noexcept { visible_ = visible; }
        [[nodiscard]] bool HasFocus() const noexcept { return focused_; }
        void SetFocus(bool focused) noexcept { focused_ = focused; }

        [[nodiscard]] const std::string& GetText() const noexcept { return text_; }
        void SetText(const std::string& text);
        void ClearText() noexcept;
        [[nodiscard]] std::size_t GetCaretPos() const noexcept { return caretPos_; }
        [[nodiscard]] bool AddCharacter(wchar_t character);
        [[nodiscard]] bool AddUtf8(std::string_view text);
        [[nodiscard]] bool Backspace();
        [[nodiscard]] bool DeleteCharacter();
        bool OnKeyDown(std::uint16_t keyCode) noexcept;

        UiRect rect_{ 0, 0, 563, 23 };

    private:
        [[nodiscard]] static std::size_t ClampToDbcsBoundary(const std::string& text, std::size_t limit) noexcept;
        [[nodiscard]] static std::size_t PreviousCharacterStart(const std::string& text, std::size_t position) noexcept;

        std::string text_;
        std::size_t caretPos_{ 0 };
        bool visible_{ false };
        bool focused_{ false };
        wchar_t pendingHighSurrogate_{ 0 };
    };
}
