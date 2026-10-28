#include "UI/Chat/ChatScrollWidget.h"

#include <algorithm>

namespace phoenix::ui
{
    void ChatScrollArea::SetItemCount(const std::size_t count) noexcept
    {
        itemCount_ = count;
        const auto maxOffset = itemCount_ == 0 ? 0 : itemCount_ - 1U;
        currentOffset_ = std::min(currentOffset_, maxOffset);
    }

    void ChatScrollArea::ScrollUp(const std::size_t amount) noexcept
    {
        const auto maxOffset = itemCount_ == 0 ? 0 : itemCount_ - 1U;
        currentOffset_ = std::min(maxOffset, currentOffset_ + amount);
    }

    void ChatScrollArea::ScrollDown(const std::size_t amount) noexcept
    {
        currentOffset_ = amount >= currentOffset_ ? 0 : currentOffset_ - amount;
    }

    void ChatScrollArea::ScrollToEnd() noexcept
    {
        currentOffset_ = 0;
    }

    std::vector<ChatMessageEntry> ChatScrollArea::FetchPage(
        const std::vector<ChatMessageEntry>& messages,
        const std::size_t pageSize) const
    {
        if (messages.empty() || pageSize == 0)
        {
            return {};
        }
        const auto end = messages.size() > currentOffset_ ? messages.size() - currentOffset_ : 0U;
        const auto begin = end > pageSize ? end - pageSize : 0U;
        return std::vector<ChatMessageEntry>(messages.begin() + static_cast<std::ptrdiff_t>(begin), messages.begin() + static_cast<std::ptrdiff_t>(end));
    }
}
