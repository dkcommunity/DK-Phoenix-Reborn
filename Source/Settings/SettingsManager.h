#pragma once

#include "Settings/SettingsTypes.h"

namespace phoenix::settings
{
    class SettingsManager final
    {
    public:
        [[nodiscard]] const ClientSettings& Current() const noexcept;
        void Apply(ClientSettings settings) noexcept;

        [[nodiscard]] static SettingChangeKind ClassifyDisplayChange(
            const DisplaySettings& before,
            const DisplaySettings& after) noexcept;

        [[nodiscard]] static DisplaySettings NormalizeDisplaySettings(DisplaySettings settings) noexcept;

        [[nodiscard]] static SettingChangeKind ClassifyAudioChange(
            const AudioSettings& before,
            const AudioSettings& after) noexcept;

    private:
        ClientSettings settings_;
    };
}
