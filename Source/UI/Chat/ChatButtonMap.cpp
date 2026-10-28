#include "UI/Chat/ChatButtonMap.h"

#include <array>

namespace phoenix::ui
{
    std::vector<ChatChannelButtonEntry> ChatButtonMap::CreateChannelDefaults()
    {
        return {
            { ChatChannelType::Normal, "Normal", 40, 41, 0 },
            { ChatChannelType::Zone, "Zone", 42, 43, 1 },
            { ChatChannelType::Whisper, "Whisper", 44, 45, 2 },
            { ChatChannelType::Party, "Party", 46, 47, 3 },
            { ChatChannelType::Guild, "Guild", 48, 49, 4 },
            { ChatChannelType::Union, "Union", 50, 51, 5 }
        };
    }

    std::vector<ChatMessageEntry> ChatButtonMap::CreateDefaultMessages()
    {
        return {};
    }

    const ChatChannelButtonEntry* ChatButtonMap::FindChannel(
        const std::vector<ChatChannelButtonEntry>& map,
        const ChatChannelType id) noexcept
    {
        for (const auto& entry : map)
        {
            if (entry.id == id)
            {
                return &entry;
            }
        }
        return nullptr;
    }

    std::string_view ChatButtonMap::Divisor(const ChatChannelType condition) noexcept
    {
        constexpr std::array<std::string_view, 9> divisors{
            ">", ">", "->", ")", "}", ":", ":", ":", ":"
        };
        const auto index = static_cast<std::size_t>(condition);
        return index < divisors.size() ? divisors[index] : divisors[0];
    }

    ChatPanelLayout ChatButtonMap::GetLayoutFromConfig(const settings::GameChatProfile& profile) noexcept
    {
        ChatPanelLayout layout;
        layout.originX = profile.originX;
        layout.originY = profile.originY;
        layout.panelWidth = profile.panelWidth;
        layout.panelHeight = profile.panelHeight;
        layout.inputHeight = profile.inputHeight;
        layout.lineHeight = profile.lineHeight;
        layout.historyLeft = profile.historyLeft;
        layout.historyRight = profile.historyRight;
        layout.historyBottomOffset = profile.historyBottomOffset;
        layout.inputLeft = profile.inputLeft;
        layout.inputTextLeft = profile.inputTextLeft;
        layout.inputTextBottomOffset = profile.inputTextBottomOffset;
        layout.dragHandleWidth = profile.dragHandleWidth;
        layout.channelButtonLeft = profile.channelButtonLeft;
        layout.channelButtonTop = profile.channelButtonTop;
        layout.channelButtonGap = profile.channelButtonGap;

        // Button rects remain hardcoded for now (they're relative to the panel,
        // not dependent on position/size). Can be config-driven later if needed.
        layout.showChannelsButton = UiRect{ 2, 27, 27, 27 };
        layout.scrollUpButton = UiRect{ 5, 55, 20, 20 };
        layout.scrollDownButton = UiRect{ 5, 78, 20, 20 };
        layout.scrollToEndButton = UiRect{ 5, 112, 20, 20 };
        layout.inputModeButton = UiRect{ 5, 135, 20, 20 };
        layout.markButton = UiRect{ 33, 130, 26, 24 };
        layout.colorButton = UiRect{ 61, 130, 26, 24 };
        layout.inputTypeButton = UiRect{ 89, 130, 26, 24 };
        layout.settingsButton = UiRect{ 117, 132, 20, 20 };

        return layout;
    }
}
