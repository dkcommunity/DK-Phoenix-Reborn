#pragma once

#include "Core/StrongId.h"
#include "Render/RenderTypes.h"
#include "UI/Layout/LayoutViewport.h"
#include "UI/UiTypes.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace phoenix::ui
{
    enum class LoginTitleResolution
    {
        R1280x960,
        R1280x1024
    };

    enum class LoginTitleSpriteRole
    {
        Background,
        ButtonLayout,
        TextPanel,
        AnimationMark,
        LoginButton,
        NewAccountButton,
        OptionButton,
        CreditsButton,
        ExitButton
    };

    enum class LoginTitleSpriteSource
    {
        RenewalGameTitle,
        TitleMenuDefault
    };

    struct LoginTitleSpriteBinding
    {
        LoginTitleSpriteRole role{ LoginTitleSpriteRole::Background };
        LoginTitleSpriteSource source{ LoginTitleSpriteSource::RenewalGameTitle };
        core::SpriteId spriteId;
        UiRect rect;
        std::wstring_view description;
    };

    struct LoginTitleLayout
    {
        LoginTitleResolution resolution{ LoginTitleResolution::R1280x960 };
        std::uint32_t width{ 1280 };
        std::uint32_t height{ 960 };
        ReferenceCanvas referenceCanvas{ 1280, 960 };
        AspectRatioPolicy backgroundAspectPolicy{ AspectRatioPolicy::Fit };
        UiRect idEdit;
        UiRect passwordEdit;
        UiRect buttonLayout;
        UiRect loginButton;
        UiRect newAccountButton;
        UiRect optionButton;
        UiRect creditsButton;
        UiRect exitButton;
        UiRect warningText;
        std::vector<LoginTitleSpriteBinding> sprites;
    };

    class LoginTitleLayoutCatalog final
    {
    public:
        [[nodiscard]] static LoginTitleLayout Create(LoginTitleResolution resolution);
        [[nodiscard]] static LoginTitleResolution Select(std::uint32_t width, std::uint32_t height) noexcept;
        [[nodiscard]] static LoginTitleLayout ScaleToViewport(LoginTitleLayout layout, ScreenViewport viewport);
    };
}
