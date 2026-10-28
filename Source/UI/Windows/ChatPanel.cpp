#include "Diagnostics/Logger.h"
#include "Diagnostics/Logger.h"
#include "Diagnostics/Logger.h"
#include "UI/Windows/ChatPanel.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace phoenix::ui
{
    namespace
    {
        bool Contains(const UiRect& rect, const std::int32_t x, const std::int32_t y) noexcept
        {
            return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
        }
    }

    ChatPanel::ChatPanel()
        : rect_{ layout_.originX, layout_.originY, layout_.panelWidth, layout_.panelHeight }
    {
        filters_.fill(true);
    }

    void ChatPanel::Show(const bool visible) noexcept
    {
        visible_ = visible;
        if (!visible_)
        {
            (void)EndDrag();
        }
    }

    void ChatPanel::SetRect(const UiRect r, const std::int32_t canvasWidth, const std::int32_t canvasHeight) noexcept
    {
        rect_ = r;
        viewportBounds_ = UiRect{ 0, 0, canvasWidth, canvasHeight };
        ClampRect(viewportBounds_);
        RefreshDockEdge();
    }

    void ChatPanel::LoadLayoutFromConfig(const settings::GameChatProfile& profile,
                                          const std::int32_t canvasWidth,
                                          const std::int32_t canvasHeight) noexcept
    {
        layout_ = ChatButtonMap::GetLayoutFromConfig(profile);
        rect_ = UiRect{ layout_.originX, layout_.originY, layout_.panelWidth, layout_.panelHeight };
        visible_ = profile.visible;
        viewportBounds_ = UiRect{ 0, 0, canvasWidth, canvasHeight };
        ClampRect(viewportBounds_);
        RefreshDockEdge();
    }

    void ChatPanel::SetActiveChannel(const ChatChannelType type)
    {
        const auto value = static_cast<std::size_t>(type);
        if (value >= LegacyChatModeCount)
        {
            return;
        }
        if (type == ChatChannelType::Whisper)
        {
            SetWhisperMode(!whisperMode_);
            return;
        }
        if (whisperMode_)
        {
            SetWhisperMode(false);
        }
        activeChannel_ = type;
        previousChannel_ = type;
    }

    void ChatPanel::SetWhisperMode(const bool enabled)
    {
        if (enabled == whisperMode_)
        {
            return;
        }
        whisperMode_ = enabled;
        whisperFocus_ = false;
        input_.ClearText();
        if (enabled)
        {
            previousChannel_ = activeChannel_;
            activeChannel_ = ChatChannelType::Whisper;
            whisperIndex_ = whisperIds_.empty() ? -1 : static_cast<std::int32_t>(whisperIds_.size() - 1U);
            whisperTarget_ = GetWhisperId();
            whisperFocus_ = whisperTarget_.empty();
        }
        else
        {
            activeChannel_ = previousChannel_;
            whisperIndex_ = -1;
        }
        input_.SetFocus(true);
    }

    void ChatPanel::SetWhisperTarget(std::string target)
    {
        whisperTarget_ = NormalizeId(target);
        if (!whisperTarget_.empty())
        {
            (void)AddWhisperId(whisperTarget_);
        }
    }

    bool ChatPanel::AddWhisperId(const std::string_view id)
    {
        auto normalized = NormalizeId(id);
        if (normalized.empty() || normalized.front() == '[')
        {
            return false;
        }
        const auto existing = std::find(whisperIds_.begin(), whisperIds_.end(), normalized);
        if (existing != whisperIds_.end())
        {
            whisperIds_.erase(existing);
        }
        if (whisperIds_.size() == WhisperIdLimit)
        {
            whisperIds_.erase(whisperIds_.begin());
        }
        whisperIds_.push_back(normalized);
        whisperIndex_ = static_cast<std::int32_t>(whisperIds_.size() - 1U);
        whisperTarget_ = whisperIds_.back();
        return true;
    }

    std::string ChatPanel::GetWhisperId(const std::int32_t index) const
    {
        if (whisperIds_.empty())
        {
            return {};
        }
        if (index < 0)
        {
            return whisperIndex_ < 0 ? whisperIds_.back() : whisperIds_[static_cast<std::size_t>(whisperIndex_)];
        }
        const auto value = static_cast<std::size_t>(index);
        return value < whisperIds_.size() ? whisperIds_[value] : std::string{};
    }

    bool ChatPanel::PreviousWhisperId()
    {
        if (whisperIds_.empty())
        {
            return false;
        }
        if (whisperIndex_ > 0)
        {
            --whisperIndex_;
        }
        else if (whisperIndex_ == -1)
        {
            whisperIndex_ = static_cast<std::int32_t>(whisperIds_.size() - 1U);
        }
        whisperTarget_ = GetWhisperId();
        return !whisperTarget_.empty();
    }

    bool ChatPanel::NextWhisperId()
    {
        if (whisperIds_.empty() || whisperIndex_ < 0)
        {
            return false;
        }
        if (whisperIndex_ < static_cast<std::int32_t>(whisperIds_.size() - 1U))
        {
            ++whisperIndex_;
            whisperTarget_ = GetWhisperId();
            return true;
        }
        whisperIndex_ = -1;
        whisperTarget_.clear();
        return false;
    }

    void ChatPanel::AddMessage(
        std::string sender,
        std::string message,
        const ChatChannelType channel,
        const std::uint32_t color)
    {
        if (message.empty())
        {
            return;
        }
        ChatMessageEntry entry{
            std::move(sender),
            std::move(message),
            channel,
            color,
            nextSequence_++
        };
        if (!scrollArea_.IsAtEnd())
        {
            if (pendingMessages_.size() == PendingHistoryLimit)
            {
                pendingMessages_.pop_front();
            }
            pendingMessages_.push_back(std::move(entry));
            return;
        }
        AddToMainHistory(std::move(entry));
    }

    void ChatPanel::AddMessage(std::string sender, std::string message, const std::uint32_t channel)
    {
        const auto bounded = std::min<std::uint32_t>(channel, static_cast<std::uint32_t>(ChatChannelType::Master));
        AddMessage(std::move(sender), std::move(message), static_cast<ChatChannelType>(bounded), 0xFFFFFFFFU);
    }

    std::vector<ChatMessageEntry> ChatPanel::VisibleMessages(const std::size_t pageSize) const
    {
        std::vector<ChatMessageEntry> filtered;
        filtered.reserve(messages_.size());
        for (const auto& message : messages_)
        {
            const auto index = static_cast<std::size_t>(message.condition);
            if (index < filters_.size() && !filters_[index])
            {
                continue;
            }
            filtered.push_back(message);
        }
        return scrollArea_.FetchPage(filtered, pageSize);
    }

    void ChatPanel::ClearMessages() noexcept
    {
        messages_.clear();
        pendingMessages_.clear();
        scrollArea_.SetItemCount(0);
        scrollArea_.ScrollToEnd();
    }

    void ChatPanel::RestorePendingMessages()
    {
        while (!pendingMessages_.empty())
        {
            auto entry = std::move(pendingMessages_.front());
            pendingMessages_.pop_front();
            AddToMainHistory(std::move(entry));
        }
        scrollArea_.ScrollToEnd();
    }

    void ChatPanel::SetFilter(const ChatChannelType channel, const bool enabled) noexcept
    {
        const auto index = static_cast<std::size_t>(channel);
        if (index < filters_.size())
        {
            filters_[index] = enabled;
            RefreshScrollCount();
            scrollArea_.ScrollToEnd();
        }
    }

    void ChatPanel::ToggleFilter(const ChatChannelType channel) noexcept
    {
        const auto index = static_cast<std::size_t>(channel);
        if (index < filters_.size())
        {
            filters_[index] = !filters_[index];
            RefreshScrollCount();
            scrollArea_.ScrollToEnd();
        }
    }

    bool ChatPanel::FilterEnabled(const ChatChannelType channel) const noexcept
    {
        const auto index = static_cast<std::size_t>(channel);
        return index >= filters_.size() || filters_[index];
    }

    void ChatPanel::ScrollUp(const std::size_t amount) noexcept
    {
        scrollArea_.ScrollUp(amount);
    }

    void ChatPanel::ScrollDown(const std::size_t amount)
    {
        const auto wasScrolled = !scrollArea_.IsAtEnd();
        scrollArea_.ScrollDown(amount);
        if (wasScrolled && scrollArea_.IsAtEnd())
        {
            RestorePendingMessages();
        }
    }

    void ChatPanel::ScrollToEnd()
    {
        RestorePendingMessages();
    }

    bool ChatPanel::RecordSentMessage(
        const std::string& message,
        const std::uint64_t nowMs,
        const bool operatorOrMaster)
    {
        if (message.empty())
        {
            return true;
        }
        if (sentHistoryCursor_ == sentHistory_.size())
        {
            if (sentHistory_.size() == SentHistoryLimit)
            {
                sentHistory_.erase(sentHistory_.begin());
            }
            sentHistory_.push_back(SentHistoryEntry{ message, { nowMs } });
        }
        else
        {
            auto entry = std::move(sentHistory_[sentHistoryCursor_]);
            if (!operatorOrMaster &&
                message.find("*command") == std::string::npos &&
                entry.times.size() == 5U &&
                entry.times.front() + PaperingMuteMs > nowMs)
            {
                paperingBlockedUntilMs_ = nowMs + PaperingMuteMs;
                return false;
            }
            if (entry.times.size() == 5U)
            {
                entry.times.pop_front();
            }
            sentHistory_.erase(sentHistory_.begin() + static_cast<std::ptrdiff_t>(sentHistoryCursor_));
            entry.message = message;
            entry.times.push_back(nowMs);
            sentHistory_.push_back(std::move(entry));
        }
        sentHistoryCursor_ = sentHistory_.size();
        return true;
    }

    std::string ChatPanel::PreviousSentMessage()
    {
        if (sentHistoryCursor_ == 0 || sentHistory_.empty())
        {
            return {};
        }
        --sentHistoryCursor_;
        return sentHistory_[sentHistoryCursor_].message;
    }

    std::string ChatPanel::NextSentMessage()
    {
        if (sentHistoryCursor_ + 1U < sentHistory_.size())
        {
            ++sentHistoryCursor_;
            return sentHistory_[sentHistoryCursor_].message;
        }
        sentHistoryCursor_ = sentHistory_.size();
        return {};
    }

    ChatPanel::SendBlockReason ChatPanel::CurrentBlockReason(
        const std::uint64_t nowMs,
        const bool zoneChat,
        const bool operatorOrMaster,
        const bool ghost,
        const bool survivalZone) const noexcept
    {
        if (ghost)
        {
            return SendBlockReason::None;
        }
        (void)operatorOrMaster;
        if (survivalZone)
        {
            return SendBlockReason::SurvivalZone;
        }
        if (repeatBlockedUntilMs_ > nowMs)
        {
            return SendBlockReason::Repeat;
        }
        if (paperingBlockedUntilMs_ > nowMs)
        {
            return SendBlockReason::Papering;
        }
        if (zoneChat && zoneBlockedUntilMs_ > nowMs)
        {
            return SendBlockReason::ZoneDelay;
        }

        return SendBlockReason::None;
    }

    ChatPanel::SendBlockReason ChatPanel::RecordChatAttempt(
        const std::uint64_t nowMs,
        const bool zoneChat,
        const bool operatorOrMaster,
        const bool commandBypass)
    {
        while (!repeatTimes_.empty() && repeatTimes_.front() + 2000ULL <= nowMs)
        {
            repeatTimes_.pop_front();
        }
        if (!operatorOrMaster && !commandBypass && repeatTimes_.size() == 5U)
        {
            repeatBlockedUntilMs_ = nowMs + RepeatMuteMs;
            return SendBlockReason::Repeat;
        }

        if (repeatTimes_.size() == 5U)
        {
            repeatTimes_.pop_front();
        }
        repeatTimes_.push_back(nowMs);
        if (zoneChat)
        {
            zoneBlockedUntilMs_ = nowMs + ZoneChatDelayMs;
        }
        return SendBlockReason::None;
    }

    bool ChatPanel::SendBlocked(const std::uint64_t nowMs, const bool zoneChat) const noexcept
    {
        return repeatBlockedUntilMs_ > nowMs || paperingBlockedUntilMs_ > nowMs ||
            (zoneChat && zoneBlockedUntilMs_ > nowMs);
    }

    bool ChatPanel::AddId(const std::string_view id)
    {
        const auto normalized = NormalizeId(id);
        return !normalized.empty() && chatIds_.insert(normalized).second;
    }

    bool ChatPanel::RemoveId(const std::string_view id)
    {
        const auto normalized = NormalizeId(id);
        return !normalized.empty() && chatIds_.erase(normalized) != 0U;
    }

    bool ChatPanel::IsAcceptedId(const std::string_view id) const
    {
        const auto normalized = NormalizeId(id);
        const auto found = chatIds_.find(normalized) != chatIds_.end();
        return ignoreMode_ ? found : !found;
    }

    void ChatPanel::SetHelpMessages(std::vector<std::string> messages)
    {
        helpMessages_ = std::move(messages);
        helpUsed_.assign(helpMessages_.size(), false);
        helpRemaining_ = helpMessages_.size();
        nextHelpMs_ = 0;
    }

    std::string ChatPanel::NextHelpMessage(const std::uint64_t nowMs, const bool enabled)
    {
        if (!enabled || helpMessages_.empty() || nowMs < nextHelpMs_)
        {
            return {};
        }
        nextHelpMs_ = nowMs + HelpDelayMs;
        if (helpRemaining_ == 0)
        {
            std::fill(helpUsed_.begin(), helpUsed_.end(), false);
            helpRemaining_ = helpMessages_.size();
        }
        auto candidate = static_cast<std::size_t>(std::rand()) % helpRemaining_;
        for (std::size_t index = 0; index < helpUsed_.size(); ++index)
        {
            if (!helpUsed_[index])
            {
                if (candidate == 0U)
                {
                    helpUsed_[index] = true;
                    --helpRemaining_;
                    return helpMessages_[index];
                }
                --candidate;
            }
        }
        return {};
    }

    void ChatPanel::ToggleSubWindow(const SubWindow window) noexcept
    {
        subWindow_ = subWindow_ == window ? SubWindow::None : window;
    }

    bool ChatPanel::BeginResize(const std::int32_t screenX, const std::int32_t screenY) noexcept
    {
        if (!visible_ || dragState_.dragging || dragState_.resizing)
        {
            return false;
        }

        // Legacy window mouse control only evaluates resize edges after the
        // pointer has hit the window itself. Without this gate, an edge's X or
        // Y strip can capture clicks that are outside the chat panel.
        if (!Contains(rect_, screenX, screenY))
        {
            return false;
        }

        constexpr std::int32_t edge = 5;
        const auto left = screenX >= rect_.x && screenX < rect_.x + edge;
        const auto right = screenX >= rect_.x + rect_.width - edge && screenX < rect_.x + rect_.width;
        const auto top = screenY >= rect_.y && screenY < rect_.y + edge;
        const auto bottom = screenY >= rect_.y + rect_.height - edge && screenY < rect_.y + rect_.height;
        resizeEdge_ = left && top ? ResizeEdge::LeftTop :
            (right && top ? ResizeEdge::RightTop :
            (left && bottom ? ResizeEdge::LeftBottom :
            (right && bottom ? ResizeEdge::RightBottom :
            (left ? ResizeEdge::Left : (right ? ResizeEdge::Right :
            (top ? ResizeEdge::Top : (bottom ? ResizeEdge::Bottom : ResizeEdge::None)))))));
        if (resizeEdge_ != ResizeEdge::None)
        {
            dragState_.resizing = true;
            dragState_.startScreenX = screenX;
            dragState_.startScreenY = screenY;
            dragState_.panelStartRect = rect_;
            return true;
        }

        return false;
    }

    bool ChatPanel::BeginDrag(const std::int32_t screenX, const std::int32_t screenY) noexcept
    {
        if (!visible_ || dragState_.dragging || dragState_.resizing)
        {
            return false;
        }

        if (BeginResize(screenX, screenY))
        {
            return true;
        }

        if (!Contains(rect_, screenX, screenY))
        {
            return false;
        }

        const UiRect dragHandle{
            rect_.x,
            rect_.y,
            std::min(rect_.width, layout_.dragHandleWidth),
            rect_.height
        };
        if (Contains(dragHandle, screenX, screenY))
        {
            dragState_.dragging = true;
            dragState_.startScreenX = screenX;
            dragState_.startScreenY = screenY;
            dragState_.panelStartRect = rect_;
            return true;
        }
        return false;
    }

    void ChatPanel::UpdateDrag(
        const std::int32_t screenX,
        const std::int32_t screenY,
        const UiRect viewportBounds) noexcept
    {
        viewportBounds_ = viewportBounds;
        if (dragState_.dragging)
        {
            rect_.x = dragState_.panelStartRect.x + screenX - dragState_.startScreenX;
            rect_.y = dragState_.panelStartRect.y + screenY - dragState_.startScreenY;
        }
        else if (dragState_.resizing)
        {
            const auto dx = screenX - dragState_.startScreenX;
            const auto dy = screenY - dragState_.startScreenY;
            auto next = dragState_.panelStartRect;
            const auto resizeLeft = resizeEdge_ == ResizeEdge::Left || resizeEdge_ == ResizeEdge::LeftTop || resizeEdge_ == ResizeEdge::LeftBottom;
            const auto resizeRight = resizeEdge_ == ResizeEdge::Right || resizeEdge_ == ResizeEdge::RightTop || resizeEdge_ == ResizeEdge::RightBottom;
            const auto resizeTop = resizeEdge_ == ResizeEdge::Top || resizeEdge_ == ResizeEdge::LeftTop || resizeEdge_ == ResizeEdge::RightTop;
            const auto resizeBottom = resizeEdge_ == ResizeEdge::Bottom || resizeEdge_ == ResizeEdge::LeftBottom || resizeEdge_ == ResizeEdge::RightBottom;
            // Left resize keeps the right edge fixed. All panel-relative
            // history and input geometry therefore follows the same rect.
            if (resizeLeft) { next.x += dx; next.width -= dx; }
            if (resizeRight) next.width += dx;
            // Top resize keeps the bottom edge fixed. Bottom-anchored input
            // controls do not move; only the history viewport height changes.
            if (resizeTop) { next.y += dy; next.height -= dy; }
            if (resizeBottom) next.height += dy;
            if (next.width < layout_.panelWidth) { if (resizeLeft) next.x -= layout_.panelWidth - next.width; next.width = layout_.panelWidth; }
            if (next.height < layout_.panelHeight) { if (resizeTop) next.y -= layout_.panelHeight - next.height; next.height = layout_.panelHeight; }
            rect_ = next;
        }
        ClampRect(viewportBounds_);
    }

    bool ChatPanel::EndDrag() noexcept
    {
        const auto changed = IsDragging() &&
            (rect_.x != dragState_.panelStartRect.x ||
                rect_.y != dragState_.panelStartRect.y ||
                rect_.width != dragState_.panelStartRect.width ||
                rect_.height != dragState_.panelStartRect.height);
        dragState_ = {};
        resizeEdge_ = ResizeEdge::None;
        RefreshDockEdge();
        return changed;
    }

    bool ChatPanel::HitTest(const std::int32_t screenX, const std::int32_t screenY) const noexcept
    {
        return visible_ && Contains(rect_, screenX, screenY);
    }

    void ChatPanel::ProcessAutoHide(
        const std::int32_t pointerX,
        const std::int32_t pointerY,
        const bool smooth,
        const UiRect viewportBounds) noexcept
    {
        viewportBounds_ = viewportBounds;
        if (!visible_ || IsDragging())
        {
            return;
        }

        constexpr std::int32_t hideGap = 4;
        constexpr std::int32_t hideSpeed = 10;
        const auto pointerInside = Contains(rect_, pointerX, pointerY);
        const auto stepToward = [smooth](const std::int32_t current, const std::int32_t target) {
            if (!smooth)
            {
                return target;
            }
            return current < target
                ? std::min(target, current + hideSpeed)
                : std::max(target, current - hideSpeed);
        };

        if (dockEdge_ == DockEdge::Top)
        {
            rect_.y = stepToward(rect_.y, pointerInside ? viewportBounds_.y : viewportBounds_.y + hideGap - rect_.height);
        }
        else if (dockEdge_ == DockEdge::Bottom)
        {
            const auto bottom = viewportBounds_.y + viewportBounds_.height;
            rect_.y = stepToward(rect_.y, pointerInside ? bottom - rect_.height : bottom - hideGap);
        }

        rect_.x = std::clamp(
            rect_.x,
            viewportBounds_.x,
            std::max(viewportBounds_.x, viewportBounds_.x + viewportBounds_.width - rect_.width));
    }

    void ChatPanel::OnViewportChanged(const UiRect viewportBounds) noexcept
    {
        const auto previousBounds = viewportBounds_;
        const auto previousBottom = previousBounds.y + previousBounds.height;
        const auto topHidden = dockEdge_ == DockEdge::Top && rect_.y < previousBounds.y;
        const auto bottomHidden = dockEdge_ == DockEdge::Bottom && rect_.y > previousBottom - rect_.height;
        viewportBounds_ = viewportBounds;
        const auto bottom = viewportBounds_.y + viewportBounds_.height;
        if (dockEdge_ == DockEdge::Top)
        {
            rect_.y = topHidden ? viewportBounds_.y + 4 - rect_.height : viewportBounds_.y;
        }
        else if (dockEdge_ == DockEdge::Bottom)
        {
            rect_.y = bottomHidden ? bottom - 4 : bottom - rect_.height;
        }
        else
        {
            ClampRect(viewportBounds_);
        }
        rect_.x = std::clamp(
            rect_.x,
            viewportBounds_.x,
            std::max(viewportBounds_.x, viewportBounds_.x + viewportBounds_.width - rect_.width));
    }

    void ChatPanel::ClampRect(const UiRect viewportBounds) noexcept
    {
        const auto boundedWidth = std::max(1, viewportBounds.width);
        const auto boundedHeight = std::max(1, viewportBounds.height);
        const auto minimumWidth = std::min(layout_.panelWidth, boundedWidth);
        const auto minimumHeight = std::min(layout_.panelHeight, boundedHeight);
        rect_.width = std::clamp(rect_.width, minimumWidth, boundedWidth);
        rect_.height = std::clamp(rect_.height, minimumHeight, boundedHeight);
        rect_.x = std::clamp(rect_.x, viewportBounds.x, viewportBounds.x + boundedWidth - rect_.width);
        rect_.y = std::clamp(rect_.y, viewportBounds.y, viewportBounds.y + boundedHeight - rect_.height);
    }

    void ChatPanel::RefreshDockEdge() noexcept
    {
        if (rect_.y <= viewportBounds_.y)
        {
            dockEdge_ = DockEdge::Top;
        }
        else if (rect_.y + rect_.height >= viewportBounds_.y + viewportBounds_.height)
        {
            dockEdge_ = DockEdge::Bottom;
        }
        else
        {
            dockEdge_ = DockEdge::None;
        }
    }

    std::string ChatPanel::NormalizeId(const std::string_view id)
    {
        auto begin = id.begin();
        auto end = id.end();
        while (begin != end && std::isspace(static_cast<unsigned char>(*begin)) != 0)
        {
            ++begin;
        }
        while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1))) != 0)
        {
            --end;
        }
        std::string normalized(begin, end);
        if (const auto space = normalized.find(' '); space != std::string::npos)
        {
            normalized.resize(space);
        }
        if (normalized.size() > 14U)
        {
            normalized.resize(14U);
        }
        return normalized;
    }

    void ChatPanel::AddToMainHistory(ChatMessageEntry entry)
    {
        if (messages_.size() == HistoryLimit)
        {
            messages_.erase(messages_.begin());
        }
        messages_.push_back(std::move(entry));
        RefreshScrollCount();
    }

    void ChatPanel::RefreshScrollCount() noexcept
    {
        const auto visibleCount = static_cast<std::size_t>(std::count_if(
            messages_.begin(),
            messages_.end(),
            [this](const ChatMessageEntry& message) {
                const auto index = static_cast<std::size_t>(message.condition);
                return index >= filters_.size() || filters_[index];
            }));
        scrollArea_.SetItemCount(visibleCount);
    }
}
