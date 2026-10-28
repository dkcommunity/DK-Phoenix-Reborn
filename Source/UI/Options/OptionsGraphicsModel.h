#pragma once

#include "Render/ResolutionPolicy.h"
#include "Settings/SettingsManager.h"
#include "Settings/SettingsTypes.h"
#include "UI/Layout/FrontendLayoutCatalog.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace phoenix::ui
{
    enum class OptionsGraphicsControlKind
    {
        ResolutionButton,
        ApplyButton,
        CancelButton
    };

    struct OptionsGraphicsControl
    {
        OptionsGraphicsControlKind kind{ OptionsGraphicsControlKind::ResolutionButton };
        UiRect rect;
        std::uint32_t width{ 1920 };
        std::uint32_t height{ 1080 };
        bool selected{ false };
        std::wstring_view label;
    };

    struct GraphicsResolutionSelection
    {
        bool accepted{ false };
        settings::DisplaySettings display;
        settings::SettingChangeKind changeKind{ settings::SettingChangeKind::Runtime };
        render::ResolutionChangePlan resolutionPlan;
        std::array<FrontendLayout, 4> layouts;
    };

    class OptionsGraphicsModel final
    {
    public:
        explicit OptionsGraphicsModel(settings::DisplaySettings current) noexcept;

        [[nodiscard]] std::vector<OptionsGraphicsControl> BuildControls(FrontendLayoutResolution resolution) const;
        [[nodiscard]] GraphicsResolutionSelection SelectResolution(std::uint32_t width, std::uint32_t height) const;

    private:
        settings::DisplaySettings current_;
    };
}
