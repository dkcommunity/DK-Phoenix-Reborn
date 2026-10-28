#pragma once

#include "UI/UiTypes.h"
#include "Settings/InterfaceProfileLoader.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace phoenix::ui
{
    enum class ChatChannelType : std::uint8_t
    {
        Normal = 0,
        Zone = 1,
        Whisper = 2,
        Party = 3,
        Guild = 4,
        Union = 5,
        Info = 6,
        GradeDisplay = 7,
        Master = 8
    };

    inline constexpr std::size_t LegacyChatFilterCount = 8;
    inline constexpr std::size_t LegacyChatModeCount = 6;

    enum class ChatSpriteFrame : std::uint16_t
    {
        Main = 9,
        PopupBackground = 21,
        InputLeft = 18,
        InputMain = 19,
        InputRight = 20,
        ScrollUp = 24,
        ScrollUpPressed = 25,
        ScrollDown = 26,
        ScrollDownPressed = 27,
        ScrollToEnd = 28,
        ScrollToEndPressed = 29,
        ShowChannels = 30,
        ShowChannelsPressed = 31,
        InputMode = 32,
        InputModePressed = 33,
        Settings = 34,
        SettingsPressed = 35,
        OpenChat = 36,
        CloseChat = 37,
        Mark = 54,
        Color = 55,
        InputType = 56
    };

    struct ChatChannelButtonEntry
    {
        ChatChannelType id{ ChatChannelType::Normal };
        const char* label{ nullptr };
        std::uint32_t normalSpriteIndex{ 0 };
        std::uint32_t highlightedSpriteIndex{ 0 };
        std::uint8_t sortOrder{ 0 };
    };

    struct ChatMessageEntry
    {
        std::string senderName;
        std::string message;
        ChatChannelType condition{ ChatChannelType::Normal };
        std::uint32_t color{ 0xFFFFFFFFU };
        std::uint64_t sequence{ 0 };
    };

    struct ChatPanelLayout
    {
        std::int32_t originX{ 0 };
        std::int32_t originY{ 546 };
        std::int32_t panelWidth{ 576 };
        std::int32_t panelHeight{ 161 };
        std::int32_t inputHeight{ 38 };
        std::int32_t lineHeight{ 16 };
        std::int32_t historyLeft{ 35 };
        std::int32_t historyRight{ 34 };
        std::int32_t historyBottomOffset{ 52 };
        std::int32_t inputLeft{ 145 };
        std::int32_t inputTextLeft{ 237 };
        std::int32_t inputTextBottomOffset{ 28 };
        std::int32_t dragHandleWidth{ 30 };
        std::int32_t channelButtonLeft{ 34 };
        std::int32_t channelButtonTop{ 19 };
        std::int32_t channelButtonGap{ 34 };
        UiRect showChannelsButton{ 2, 27, 27, 27 };
        UiRect scrollUpButton{ 5, 55, 20, 20 };
        UiRect scrollDownButton{ 5, 78, 20, 20 };
        UiRect scrollToEndButton{ 5, 112, 20, 20 };
        UiRect inputModeButton{ 5, 135, 20, 20 };
        UiRect markButton{ 33, 130, 26, 24 };
        UiRect colorButton{ 61, 130, 26, 24 };
        UiRect inputTypeButton{ 89, 130, 26, 24 };
        UiRect settingsButton{ 117, 132, 20, 20 };
    };

    class ChatButtonMap final
    {
    public:
        static std::vector<ChatChannelButtonEntry> CreateChannelDefaults();
        static std::vector<ChatMessageEntry> CreateDefaultMessages();
        static const ChatChannelButtonEntry* FindChannel(
            const std::vector<ChatChannelButtonEntry>& map,
            ChatChannelType id) noexcept;
        [[nodiscard]] static std::string_view Divisor(ChatChannelType condition) noexcept;

        // Build layout from config profile (preferred), falls back to defaults
        [[nodiscard]] static ChatPanelLayout GetLayoutFromConfig(const settings::GameChatProfile& profile) noexcept;
        [[nodiscard]] static ChatPanelLayout GetLayout() noexcept { return {}; }
    };
}
