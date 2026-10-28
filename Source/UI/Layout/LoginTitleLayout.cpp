#include "UI/Layout/LoginTitleLayout.h"

namespace phoenix::ui
{
    namespace
    {
        constexpr std::int32_t kEditWidth = 130;
        constexpr std::int32_t kEditHeight = 23;
        constexpr std::int32_t kMenuButtonWidth = 74;
        constexpr std::int32_t kMenuButtonHeight = 22;

        LoginTitleSpriteBinding MakeBinding(
            const LoginTitleSpriteRole role,
            const LoginTitleSpriteSource source,
            const std::uint32_t spriteId,
            const UiRect rect,
            const std::wstring_view description)
        {
            return LoginTitleSpriteBinding{ role, source, core::SpriteId{ spriteId }, rect, description };
        }

        void AddCommonBindings(LoginTitleLayout& layout, const std::uint32_t backgroundSpriteId)
        {
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::Background, LoginTitleSpriteSource::RenewalGameTitle, backgroundSpriteId, UiRect{ 0, 0, static_cast<std::int32_t>(layout.width), static_cast<std::int32_t>(layout.height) }, L"renewalgametitle background"));
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::ButtonLayout, LoginTitleSpriteSource::RenewalGameTitle, 4, layout.buttonLayout, L"login username/password panel"));
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::LoginButton, LoginTitleSpriteSource::TitleMenuDefault, 1, layout.loginButton, L"connect/login button normal frame from TitleMenuDefault"));
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::NewAccountButton, LoginTitleSpriteSource::TitleMenuDefault, 9, layout.newAccountButton, L"new account button normal frame from TitleMenuDefault"));
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::OptionButton, LoginTitleSpriteSource::TitleMenuDefault, 3, layout.optionButton, L"options button normal frame from TitleMenuDefault"));
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::CreditsButton, LoginTitleSpriteSource::TitleMenuDefault, 5, layout.creditsButton, L"credits button normal frame from TitleMenuDefault"));
            layout.sprites.push_back(MakeBinding(LoginTitleSpriteRole::ExitButton, LoginTitleSpriteSource::TitleMenuDefault, 7, layout.exitButton, L"exit button normal frame from TitleMenuDefault"));
        }
    }

    LoginTitleLayout LoginTitleLayoutCatalog::Create(const LoginTitleResolution resolution)
    {
        LoginTitleLayout layout;
        layout.resolution = resolution;

        if (resolution == LoginTitleResolution::R1280x1024)
        {
            layout.width = 1280;
            layout.height = 1024;
            layout.referenceCanvas = ReferenceCanvas{ 1280, 1024 };
            layout.idEdit = UiRect{ 908, 810, kEditWidth, kEditHeight };
            layout.passwordEdit = UiRect{ 908, 843, kEditWidth, kEditHeight };
            layout.buttonLayout = UiRect{ 840, 743, 245, 156 };
            layout.loginButton = UiRect{ 920, 866, kMenuButtonWidth, kMenuButtonHeight };
            layout.newAccountButton = UiRect{ 1150, 746, kMenuButtonWidth, kMenuButtonHeight };
            layout.optionButton = UiRect{ 1150, 788, kMenuButtonWidth, kMenuButtonHeight };
            layout.creditsButton = UiRect{ 1150, 830, kMenuButtonWidth, kMenuButtonHeight };
            layout.exitButton = UiRect{ 1150, 874, kMenuButtonWidth, kMenuButtonHeight };
            layout.warningText = UiRect{ 485, 948, 332, 24 };
            AddCommonBindings(layout, 3);
            return layout;
        }

        layout.width = 1280;
        layout.height = 960;
        layout.referenceCanvas = ReferenceCanvas{ 1280, 960 };
        layout.idEdit = UiRect{ 970, 745, kEditWidth, kEditHeight };
        layout.passwordEdit = UiRect{ 970, 780, kEditWidth, kEditHeight };
        layout.buttonLayout = UiRect{ 900, 679, 245, 156 };
        layout.loginButton = UiRect{ 990, 805, kMenuButtonWidth, kMenuButtonHeight };
        layout.newAccountButton = UiRect{ 1185, 680, kMenuButtonWidth, kMenuButtonHeight };
        layout.optionButton = UiRect{ 1185, 720, kMenuButtonWidth, kMenuButtonHeight };
        layout.creditsButton = UiRect{ 1185, 763, kMenuButtonWidth, kMenuButtonHeight };
        layout.exitButton = UiRect{ 1185, 805, kMenuButtonWidth, kMenuButtonHeight };
        layout.warningText = UiRect{ 485, 916, 332, 24 };
        AddCommonBindings(layout, 3);
        return layout;
    }

    LoginTitleResolution LoginTitleLayoutCatalog::Select(const std::uint32_t width, const std::uint32_t height) noexcept
    {
        if (width == 1280 && height >= 1024)
        {
            return LoginTitleResolution::R1280x1024;
        }

        return LoginTitleResolution::R1280x960;
    }

    LoginTitleLayout LoginTitleLayoutCatalog::ScaleToViewport(LoginTitleLayout layout, const ScreenViewport viewport)
    {
        const auto transform = LayoutViewport::BuildTransform(layout.referenceCanvas, viewport);
        layout.width = viewport.width;
        layout.height = viewport.height;
        layout.idEdit = LayoutViewport::ScaleRect(layout.idEdit, transform);
        layout.passwordEdit = LayoutViewport::ScaleRect(layout.passwordEdit, transform);
        layout.buttonLayout = LayoutViewport::ScaleRect(layout.buttonLayout, transform);
        layout.loginButton = LayoutViewport::ScaleRect(layout.loginButton, transform);
        layout.newAccountButton = LayoutViewport::ScaleRect(layout.newAccountButton, transform);
        layout.optionButton = LayoutViewport::ScaleRect(layout.optionButton, transform);
        layout.creditsButton = LayoutViewport::ScaleRect(layout.creditsButton, transform);
        layout.exitButton = LayoutViewport::ScaleRect(layout.exitButton, transform);
        layout.warningText = LayoutViewport::ScaleRect(layout.warningText, transform);
        for (auto& binding : layout.sprites)
        {
            if (binding.role == LoginTitleSpriteRole::Background)
            {
                binding.rect = LayoutViewport::ResolveAspectRect(layout.referenceCanvas, viewport, layout.backgroundAspectPolicy).rect;
                continue;
            }

            binding.rect = LayoutViewport::ScaleRect(binding.rect, transform);
        }

        return layout;
    }
}
