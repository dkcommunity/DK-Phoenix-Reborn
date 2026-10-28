#include "Settings/SettingsManager.h"

#include "Settings/DisplayResolutionCatalog.h"

namespace phoenix::settings
{
    const ClientSettings& SettingsManager::Current() const noexcept
    {
        return settings_;
    }

    void SettingsManager::Apply(ClientSettings settings) noexcept
    {
        settings.display = NormalizeDisplaySettings(settings.display);
        settings_ = settings;
    }

    SettingChangeKind SettingsManager::ClassifyDisplayChange(
        const DisplaySettings& before,
        const DisplaySettings& after) noexcept
    {
        if (before.fullscreen != after.fullscreen)
        {
            return SettingChangeKind::DeviceReset;
        }

        if (before.width != after.width || before.height != after.height)
        {
            return SettingChangeKind::SwapChainAndLayout;
        }

        if (before.vsync != after.vsync)
        {
            return SettingChangeKind::DeviceReset;
        }

        return SettingChangeKind::Runtime;
    }

    DisplaySettings SettingsManager::NormalizeDisplaySettings(DisplaySettings settings) noexcept
    {
        return DisplayResolutionCatalog::Normalize(settings);
    }

    SettingChangeKind SettingsManager::ClassifyAudioChange(
        const AudioSettings&,
        const AudioSettings&) noexcept
    {
        return SettingChangeKind::Runtime;
    }
}
