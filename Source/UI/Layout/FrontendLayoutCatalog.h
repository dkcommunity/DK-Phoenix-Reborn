#pragma once

#include "UI/Layout/LayoutViewport.h"
#include "UI/UiElement.h"
#include "UI/UiTypes.h"

#include <cstdint>
#include <vector>

namespace phoenix::ui
{
    enum class FrontendLayoutScreen
    {
        Login,
        ServerSelect,
        CharacterSelect,
        CharacterCreate
    };

    enum class FrontendLayoutResolution
    {
        R1920x1080,
        R1024x768
    };

    struct FrontendLayout
    {
        FrontendLayoutScreen screen{ FrontendLayoutScreen::Login };
        FrontendLayoutResolution resolution{ FrontendLayoutResolution::R1920x1080 };
        ReferenceCanvas referenceCanvas{ 1280, 960 };
        AspectRatioPolicy backgroundAspectPolicy{ AspectRatioPolicy::Fit };
        std::uint32_t width{ 1920 };
        std::uint32_t height{ 1080 };
        std::vector<UiElement> elements;
    };

    class FrontendLayoutCatalog final
    {
    public:
        [[nodiscard]] static FrontendLayout Create(FrontendLayoutScreen screen);
        [[nodiscard]] static FrontendLayout Create(FrontendLayoutScreen screen, FrontendLayoutResolution resolution);
        [[nodiscard]] static FrontendLayoutResolution SelectResolution(std::uint32_t width, std::uint32_t height) noexcept;
        [[nodiscard]] static FrontendLayout ScaleToViewport(FrontendLayout layout, ScreenViewport viewport);
    };
}
