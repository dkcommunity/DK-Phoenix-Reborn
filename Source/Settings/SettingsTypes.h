#pragma once

#include <cstdint>

namespace phoenix::settings
{
    enum class SettingChangeKind
    {
        Runtime,
        SwapChainAndLayout,
        DeviceReset,
        StartupOnly
    };

    struct DisplaySettings
    {
        std::uint32_t width{ 1920 };
        std::uint32_t height{ 1080 };
        bool fullscreen{ false };
        bool vsync{ true };
        std::uint32_t frameLimit{ 60 };
        float uiScale{ 1.0F };
    };

    struct AudioSettings
    {
        float masterVolume{ 1.0F };
        float musicVolume{ 0.8F };
        float effectsVolume{ 0.8F };
        bool musicEnabled{ true };
        bool effectsEnabled{ true };
    };

    struct MediaSettings
    {
        bool playIntroVideo{ true };
        bool animatedBackgrounds{ true };
    };

    struct ClientSettings
    {
        DisplaySettings display;
        AudioSettings audio;
        MediaSettings media;
    };
}
