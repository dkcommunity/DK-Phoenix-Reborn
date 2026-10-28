#pragma once

#include "UI/Chat/ChatButtonMap.h"
#include "UI/Chat/ChatInputField.h"
#include "UI/Chat/ChatScrollWidget.h"
#include "Settings/InterfaceProfileLoader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace phoenix::ui
{
    class ChatPanel final
    {
    public:
        static constexpr std::size_t HistoryLimit = 200;
        static constexpr std::size_t PendingHistoryLimit = 250;
        static constexpr std::size_t SentHistoryLimit = 20;
        static constexpr std::size_t WhisperIdLimit = 14;
        static constexpr std::uint64_t ZoneChatDelayMs = 15000;
        static constexpr std::uint64_t RepeatMuteMs = 20000;
        static constexpr std::uint64_t PaperingMuteMs = 20000;
        static constexpr std::uint64_t HelpDelayMs = 60000;

        struct DragState
        {
            bool dragging{ false };
            bool resizing{ false };
            std::int32_t startScreenX{ 0 };
            std::int32_t startScreenY{ 0 };
            UiRect panelStartRect{};
        };

        enum class SendBlockReason : std::uint8_t
        {
            None,
            ZoneDelay,
            Repeat,
            Papering,
            SurvivalZone,
            Muted
        };

        enum class SubWindow : std::uint8_t
        {
            None = 0,
            Mark = 1,
            Color = 2,
            Filter = 3
        };

        ChatPanel();

        void Show(bool visible) noexcept;
        [[nodiscard]] bool IsVisible() const noexcept { return visible_; }

        [[nodiscard]] UiRect GetPanelRect() const noexcept { return rect_; }
        void SetRect(UiRect r, std::int32_t canvasWidth = 1280, std::int32_t canvasHeight = 960) noexcept;

        // Load layout from config profile
        void LoadLayoutFromConfig(const settings::GameChatProfile& profile, std::int32_t canvasWidth = 1280, std::int32_t canvasHeight = 960) noexcept;

        void SetActiveChannel(ChatChannelType type);
        [[nodiscard]] ChatChannelType GetActiveChannel() const noexcept { return activeChannel_; }
        [[nodiscard]] ChatChannelType GetPreviousChannel() const noexcept { return previousChannel_; }
        void SetWhisperMode(bool enabled);
        [[nodiscard]] bool IsWhisperMode() const noexcept { return whisperMode_; }
        void SetWhisperFocus(bool focused) noexcept { whisperFocus_ = focused; }
        [[nodiscard]] bool HasWhisperFocus() const noexcept { return whisperFocus_; }
        void SetWhisperTarget(std::string target);
        [[nodiscard]] const std::string& GetWhisperTarget() const noexcept { return whisperTarget_; }
        bool AddWhisperId(std::string_view id);
        [[nodiscard]] std::string GetWhisperId(std::int32_t index = -1) const;
        [[nodiscard]] std::size_t WhisperIdCount() const noexcept { return whisperIds_.size(); }
        [[nodiscard]] bool PreviousWhisperId();
        [[nodiscard]] bool NextWhisperId();

        void AddMessage(
            std::string sender,
            std::string message,
            ChatChannelType channel,
            std::uint32_t color);
        void AddMessage(std::string sender, std::string message, std::uint32_t channel);
        [[nodiscard]] const std::vector<ChatMessageEntry>& GetMessageList() const noexcept { return messages_; }
        [[nodiscard]] std::vector<ChatMessageEntry> VisibleMessages(std::size_t pageSize) const;
        void ClearMessages() noexcept;
        void RestorePendingMessages();

        void SetFilter(ChatChannelType channel, bool enabled) noexcept;
        void ToggleFilter(ChatChannelType channel) noexcept;
        [[nodiscard]] bool FilterEnabled(ChatChannelType channel) const noexcept;
        [[nodiscard]] const std::array<bool, LegacyChatFilterCount>& Filters() const noexcept { return filters_; }

        void ScrollUp(std::size_t amount = 1) noexcept;
        void ScrollDown(std::size_t amount = 1);
        void ScrollToEnd();
        [[nodiscard]] bool IsAtHistoryEnd() const noexcept { return scrollArea_.IsAtEnd(); }
        [[nodiscard]] std::size_t HistoryOffset() const noexcept { return scrollArea_.CurrentOffset(); }

        [[nodiscard]] bool RecordSentMessage(
            const std::string& message,
            std::uint64_t nowMs,
            bool operatorOrMaster);
        [[nodiscard]] std::string PreviousSentMessage();
        [[nodiscard]] std::string NextSentMessage();
        [[nodiscard]] SendBlockReason CurrentBlockReason(
            std::uint64_t nowMs,
            bool zoneChat,
            bool operatorOrMaster,
            bool ghost,
            bool survivalZone) const noexcept;
        [[nodiscard]] SendBlockReason RecordChatAttempt(
            std::uint64_t nowMs,
            bool zoneChat,
            bool operatorOrMaster,
            bool commandBypass);
        [[nodiscard]] bool SendBlocked(std::uint64_t nowMs, bool zoneChat) const noexcept;

        void SetIgnoreMode(bool enabled) noexcept { ignoreMode_ = enabled; }
        [[nodiscard]] bool IsIgnoreMode() const noexcept { return ignoreMode_; }
        void ClearIds() noexcept { chatIds_.clear(); }
        [[nodiscard]] bool AddId(std::string_view id);
        [[nodiscard]] bool RemoveId(std::string_view id);
        [[nodiscard]] bool IsAcceptedId(std::string_view id) const;

        void SetSettingsVisible(bool visible) noexcept { subWindow_ = visible ? SubWindow::Filter : SubWindow::None; }
        void ToggleSettings() noexcept { ToggleSubWindow(SubWindow::Filter); }
        [[nodiscard]] bool SettingsVisible() const noexcept { return subWindow_ == SubWindow::Filter; }
        void ToggleSubWindow(SubWindow window) noexcept;
        [[nodiscard]] SubWindow ActiveSubWindow() const noexcept { return subWindow_; }
        void ToggleChannelButtons() noexcept { showChannelButtons_ = !showChannelButtons_; }
        [[nodiscard]] bool ChannelButtonsVisible() const noexcept { return showChannelButtons_; }
        void ToggleWhisperIds() noexcept { spreadWhisperIds_ = !spreadWhisperIds_; }
        [[nodiscard]] bool WhisperIdsVisible() const noexcept { return spreadWhisperIds_; }

        [[nodiscard]] ChatInputField& Input() noexcept { return input_; }
        [[nodiscard]] const ChatInputField& Input() const noexcept { return input_; }

        void SetHelpMessages(std::vector<std::string> messages);
        [[nodiscard]] std::string NextHelpMessage(std::uint64_t nowMs, bool enabled);

        bool BeginResize(std::int32_t screenX, std::int32_t screenY) noexcept;
        bool BeginDrag(std::int32_t screenX, std::int32_t screenY) noexcept;
        void UpdateDrag(
            std::int32_t screenX,
            std::int32_t screenY,
            UiRect viewportBounds) noexcept;
        [[nodiscard]] bool EndDrag() noexcept;
        [[nodiscard]] bool IsDragging() const noexcept { return dragState_.dragging || dragState_.resizing; }
        [[nodiscard]] bool HitTest(std::int32_t screenX, std::int32_t screenY) const noexcept;
        void ProcessAutoHide(
            std::int32_t pointerX,
            std::int32_t pointerY,
            bool smooth,
            UiRect viewportBounds) noexcept;
        void OnViewportChanged(UiRect viewportBounds) noexcept;
        [[nodiscard]] static ChatPanelLayout GetLayout() noexcept { return ChatButtonMap::GetLayout(); }

    private:
        enum class ResizeEdge : std::uint8_t
        {
            None, Left, Top, Right, Bottom, LeftTop, RightTop, LeftBottom, RightBottom
        };

        enum class DockEdge : std::uint8_t
        {
            None, Top, Bottom
        };

        struct SentHistoryEntry
        {
            std::string message;
            std::deque<std::uint64_t> times;
        };

        static std::string NormalizeId(std::string_view id);
        void ClampRect(UiRect viewportBounds) noexcept;
        void RefreshDockEdge() noexcept;
        void AddToMainHistory(ChatMessageEntry entry);
        void RefreshScrollCount() noexcept;

        UiRect rect_;
        DragState dragState_;
        ChatChannelType activeChannel_{ ChatChannelType::Normal };
        ChatChannelType previousChannel_{ ChatChannelType::Normal };
        std::vector<ChatMessageEntry> messages_;
        std::deque<ChatMessageEntry> pendingMessages_;
        std::array<bool, LegacyChatFilterCount> filters_{};
        ChatScrollArea scrollArea_;
        ChatInputField input_;
        std::vector<SentHistoryEntry> sentHistory_;
        std::size_t sentHistoryCursor_{ 0 };
        std::deque<std::uint64_t> repeatTimes_;
        std::vector<std::string> whisperIds_;
        std::int32_t whisperIndex_{ -1 };
        std::string whisperTarget_;
        bool whisperMode_{ false };
        bool whisperFocus_{ false };
        bool ignoreMode_{ false };
        std::unordered_set<std::string> chatIds_;
        std::vector<std::string> helpMessages_;
        std::vector<bool> helpUsed_;
        std::size_t helpRemaining_{ 0 };
        std::uint64_t nextHelpMs_{ 0 };
        std::uint64_t zoneBlockedUntilMs_{ 0 };
        std::uint64_t repeatBlockedUntilMs_{ 0 };
        std::uint64_t paperingBlockedUntilMs_{ 0 };
        std::uint64_t nextSequence_{ 1 };
        SubWindow subWindow_{ SubWindow::None };
        bool showChannelButtons_{ false };
        bool spreadWhisperIds_{ false };
        bool visible_{ true };
        ChatPanelLayout layout_{};
        ResizeEdge resizeEdge_{ ResizeEdge::None };
        DockEdge dockEdge_{ DockEdge::None };
        UiRect viewportBounds_{ 0, 0, 1280, 960 };
    };
}
