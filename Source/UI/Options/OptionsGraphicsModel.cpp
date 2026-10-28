#include "UI/Options/OptionsGraphicsModel.h"

#include "Settings/DisplayResolutionCatalog.h"

namespace phoenix::ui
{
    namespace
    {
        std::array<FrontendLayout, 4> BuildLayouts(const FrontendLayoutResolution resolution)
        {
            return {
                FrontendLayoutCatalog::Create(FrontendLayoutScreen::Login, resolution),
                FrontendLayoutCatalog::Create(FrontendLayoutScreen::ServerSelect, resolution),
                FrontendLayoutCatalog::Create(FrontendLayoutScreen::CharacterSelect, resolution),
                FrontendLayoutCatalog::Create(FrontendLayoutScreen::CharacterCreate, resolution)
            };
        }
    }

    OptionsGraphicsModel::OptionsGraphicsModel(settings::DisplaySettings current) noexcept
        : current_(settings::DisplayResolutionCatalog::Normalize(current))
    {
    }

    std::vector<OptionsGraphicsControl> OptionsGraphicsModel::BuildControls(const FrontendLayoutResolution resolution) const
    {
        const auto layout = FrontendLayoutCatalog::Create(FrontendLayoutScreen::Login, resolution);
        const auto selected1920 = current_.width == 1920 && current_.height == 1080;
        const auto selected1280x1024 = current_.width == 1280 && current_.height == 1024;
        const auto selected1280x960 = current_.width == 1280 && current_.height == 960;
        const auto selected1024 = current_.width == 1024 && current_.height == 768;
        const auto scaleX = static_cast<float>(layout.width) / 1920.0F;
        const auto scaleY = static_cast<float>(layout.height) / 1080.0F;
        const auto makeRect = [scaleX, scaleY](const std::int32_t x, const std::int32_t y, const std::int32_t width, const std::int32_t height) {
            return UiRect{
                static_cast<std::int32_t>(static_cast<float>(x) * scaleX),
                static_cast<std::int32_t>(static_cast<float>(y) * scaleY),
                static_cast<std::int32_t>(static_cast<float>(width) * scaleX),
                static_cast<std::int32_t>(static_cast<float>(height) * scaleY)
            };
        };

        return {
            OptionsGraphicsControl{ OptionsGraphicsControlKind::ResolutionButton, makeRect(760, 420, 180, 32), 1920, 1080, selected1920, L"1920x1080" },
            OptionsGraphicsControl{ OptionsGraphicsControlKind::ResolutionButton, makeRect(980, 420, 180, 32), 1280, 1024, selected1280x1024, L"1280x1024" },
            OptionsGraphicsControl{ OptionsGraphicsControlKind::ResolutionButton, makeRect(760, 470, 180, 32), 1280, 960, selected1280x960, L"1280x960" },
            OptionsGraphicsControl{ OptionsGraphicsControlKind::ResolutionButton, makeRect(980, 470, 180, 32), 1024, 768, selected1024, L"1024x768" },
            OptionsGraphicsControl{ OptionsGraphicsControlKind::ApplyButton, makeRect(860, 500, 120, 32), current_.width, current_.height, false, L"Apply" },
            OptionsGraphicsControl{ OptionsGraphicsControlKind::CancelButton, makeRect(1000, 500, 120, 32), current_.width, current_.height, false, L"Cancel" }
        };
    }

    GraphicsResolutionSelection OptionsGraphicsModel::SelectResolution(const std::uint32_t width, const std::uint32_t height) const
    {
        GraphicsResolutionSelection result;
        result.accepted = settings::DisplayResolutionCatalog::IsSupported(width, height);

        auto next = current_;
        next.width = width;
        next.height = height;
        next = settings::SettingsManager::NormalizeDisplaySettings(next);

        result.display = next;
        result.changeKind = settings::SettingsManager::ClassifyDisplayChange(current_, next);
        result.resolutionPlan = render::ResolutionPolicy::Plan(
            current_.width,
            current_.height,
            render::ResolutionChangeRequest{ next.width, next.height });
        result.layouts = BuildLayouts(FrontendLayoutCatalog::SelectResolution(next.width, next.height));
        return result;
    }
}
