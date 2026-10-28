#pragma once

#include "UI/Chat/ChatButtonMap.h"
#include "UI/UiTypes.h"

#include <cstddef>
#include <vector>

namespace phoenix::ui
{
    class ChatScrollArea final
    {
    public:
        void SetItemCount(std::size_t count) noexcept;
        void ScrollUp(std::size_t amount = 1) noexcept;
        void ScrollDown(std::size_t amount = 1) noexcept;
        void ScrollToEnd() noexcept;
        [[nodiscard]] std::size_t CurrentOffset() const noexcept { return currentOffset_; }
        [[nodiscard]] bool IsAtEnd() const noexcept { return currentOffset_ == 0; }
        [[nodiscard]] std::vector<ChatMessageEntry> FetchPage(
            const std::vector<ChatMessageEntry>& messages,
            std::size_t pageSize) const;

        UiRect rect_{ 0, 0, 529, 82 };

    private:
        std::size_t itemCount_{ 0 };
        std::size_t currentOffset_{ 0 };
    };
}
