#include "UI/Chat/ChatInputField.h"

#include <Windows.h>

#include <algorithm>
#include <array>

namespace phoenix::ui
{
    std::size_t ChatInputField::ClampToDbcsBoundary(const std::string& text, const std::size_t limit) noexcept
    {
        const auto bounded = std::min(limit, text.size());
        std::size_t cursor = 0;
        while (cursor < bounded)
        {
            const auto lead = static_cast<unsigned char>(text[cursor]);
            const auto width = lead < 0x80U ? 1U : (lead < 0xE0U ? 2U : (lead < 0xF0U ? 3U : 4U));
            if (cursor + width > bounded) break;
            cursor += width;
        }
        return cursor;
    }

    std::size_t ChatInputField::PreviousCharacterStart(const std::string& text, const std::size_t position) noexcept
    {
        if (position == 0 || text.empty())
        {
            return 0;
        }
        auto previous = position - 1U;
        while (previous > 0U && (static_cast<unsigned char>(text[previous]) & 0xC0U) == 0x80U) --previous;
        return previous;
    }

    void ChatInputField::SetText(const std::string& text)
    {
        const auto length = ClampToDbcsBoundary(text, LegacyByteLimit);
        text_.assign(text.data(), length);
        caretPos_ = text_.size();
    }

    void ChatInputField::ClearText() noexcept
    {
        text_.clear();
        caretPos_ = 0;
        pendingHighSurrogate_ = 0;
    }

    bool ChatInputField::AddUtf8(const std::string_view text)
    {
        if (text.empty() || text_.size() + text.size() > LegacyByteLimit)
        {
            return false;
        }
        text_.insert(caretPos_, text.data(), text.size());
        caretPos_ += text.size();
        return true;
    }

    bool ChatInputField::AddCharacter(const wchar_t character)
    {
        std::array<wchar_t, 2> utf16{};
        int utf16Length = 1;
        if (character >= 0xD800 && character <= 0xDBFF)
        {
            pendingHighSurrogate_ = character;
            return true;
        }
        if (character >= 0xDC00 && character <= 0xDFFF && pendingHighSurrogate_ != 0)
        {
            utf16[0] = pendingHighSurrogate_;
            utf16[1] = character;
            utf16Length = 2;
            pendingHighSurrogate_ = 0;
        }
        else
        {
            pendingHighSurrogate_ = 0;
            utf16[0] = character;
        }
        std::array<char, 8> encoded{};
        const auto count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), utf16Length,
            encoded.data(), static_cast<int>(encoded.size()), nullptr, nullptr);
        return count > 0 && AddUtf8(std::string_view(encoded.data(), static_cast<std::size_t>(count)));
    }

    bool ChatInputField::Backspace()
    {
        if (caretPos_ == 0 || text_.empty())
        {
            return false;
        }
        const auto start = PreviousCharacterStart(text_, caretPos_);
        text_.erase(start, caretPos_ - start);
        caretPos_ = start;
        return true;
    }

    bool ChatInputField::DeleteCharacter()
    {
        if (caretPos_ >= text_.size())
        {
            return false;
        }
        auto end = caretPos_ + 1U;
        while (end < text_.size() && (static_cast<unsigned char>(text_[end]) & 0xC0U) == 0x80U) ++end;
        text_.erase(caretPos_, end - caretPos_);
        return true;
    }

    bool ChatInputField::OnKeyDown(const std::uint16_t keyCode) noexcept
    {
        switch (keyCode)
        {
        case VK_RETURN:
            return true;
        case VK_BACK:
            (void)Backspace();
            break;
        case VK_DELETE:
            (void)DeleteCharacter();
            break;
        case VK_LEFT:
            caretPos_ = PreviousCharacterStart(text_, caretPos_);
            break;
        case VK_RIGHT:
            if (caretPos_ < text_.size())
            {
                ++caretPos_;
                while (caretPos_ < text_.size() && (static_cast<unsigned char>(text_[caretPos_]) & 0xC0U) == 0x80U) ++caretPos_;
                caretPos_ = std::min(caretPos_, text_.size());
            }
            break;
        case VK_HOME:
            caretPos_ = 0;
            break;
        case VK_END:
            caretPos_ = text_.size();
            break;
        default:
            break;
        }
        return false;
    }
}
