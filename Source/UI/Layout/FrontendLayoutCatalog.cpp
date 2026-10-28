#include "UI/Layout/FrontendLayoutCatalog.h"

namespace phoenix::ui
{
    namespace
    {
        UiElement MakeElement(const wchar_t* id, const UiRect rect, const UiAnchor anchor = UiAnchor::TopLeft)
        {
            UiElement element;
            element.id.value = id;
            element.rect = rect;
            element.anchor = anchor;
            return element;
        }

        void AddLoginElements(FrontendLayout& layout)
        {
            layout.elements.push_back(MakeElement(L"Login.IdEdit", UiRect{ 970, 745, 130, 23 }));
            layout.elements.push_back(MakeElement(L"Login.PasswordEdit", UiRect{ 970, 780, 130, 23 }));
            layout.elements.push_back(MakeElement(L"Login.ButtonPanel", UiRect{ 900, 679, 245, 156 }));
            layout.elements.push_back(MakeElement(L"Login.ConnectButton", UiRect{ 990, 805, 74, 22 }));
            layout.elements.push_back(MakeElement(L"Login.NewAccountButton", UiRect{ 1185, 680, 74, 22 }));
            layout.elements.push_back(MakeElement(L"Login.OptionButton", UiRect{ 1185, 720, 74, 22 }));
            layout.elements.push_back(MakeElement(L"Login.CreditsButton", UiRect{ 1185, 763, 74, 22 }));
            layout.elements.push_back(MakeElement(L"Login.ExitButton", UiRect{ 1185, 805, 74, 22 }));
        }

        void AddServerSelectElements(FrontendLayout& layout)
        {
            layout.elements.push_back(MakeElement(L"ServerSelect.WorldList", UiRect{ 378, 320, 169, 276 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.WorldBar", UiRect{ 406, 351, 114, 17 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.WorldNameText", UiRect{ 410, 352, 82, 16 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.WorldStatusText", UiRect{ 492, 352, 50, 16 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.ChannelList", UiRect{ 578, 320, 310, 277 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.ChannelBar", UiRect{ 605, 374, 114, 17 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.ChannelNameText", UiRect{ 610, 375, 100, 16 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.ChannelStatusText", UiRect{ 722, 375, 90, 16 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.ChannelRaceBarStart", UiRect{ 786, 378, 5, 5 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.PrevButton", UiRect{ 528, 660, 106, 27 }));
            layout.elements.push_back(MakeElement(L"ServerSelect.NextButton", UiRect{ 648, 660, 82, 27 }));
        }

        void AddCharacterSelectElements(FrontendLayout& layout)
        {
            layout.elements.push_back(MakeElement(L"CharacterSelect.Panel", UiRect{ 20, 105, 340, 560 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.InfoPanel", UiRect{ 1100, 399, 150, 180 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.LeftSlot", UiRect{ 481, 615, 70, 120 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.CenterSlot", UiRect{ 608, 615, 70, 120 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.RightSlot", UiRect{ 738, 615, 70, 120 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.LeftNameBar", UiRect{ 463, 520, 70, 24 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.CenterNameBar", UiRect{ 593, 520, 70, 24 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.RightNameBar", UiRect{ 723, 520, 70, 24 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.PrevButton", UiRect{ 840, 894, 74, 22 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.GameStartButton", UiRect{ 1075, 894, 74, 22 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.CreateButton", UiRect{ 66, 596, 74, 22 }));
            layout.elements.push_back(MakeElement(L"CharacterSelect.DeleteButton", UiRect{ 211, 596, 74, 22 }));
        }

        void AddCharacterCreateElements(FrontendLayout& layout)
        {
            layout.elements.push_back(MakeElement(L"CharacterCreate.NewCharBox", UiRect{ 898, 152, 260, 560 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.NameCheckButton", UiRect{ 1077, 176, 74, 22 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.NameFocus", UiRect{ 918, 180, 130, 23 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.LeftRotationButton", UiRect{ 919, 253, 32, 32 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.RightRotationButton", UiRect{ 993, 253, 32, 32 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.CharacterView", UiRect{ 940, 282, 120, 180 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.RaceSlayerButton", UiRect{ 181, 236, 140, 40 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.RaceVampireButton", UiRect{ 181, 335, 140, 40 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.RaceOustersButton", UiRect{ 181, 434, 140, 40 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.GenderBox", UiRect{ 914, 353, 100, 32 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.HairColorBox", UiRect{ 908, 521, 100, 32 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.SkinColorBox", UiRect{ 1040, 519, 100, 32 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.StatBox", UiRect{ 1053, 383, 120, 140 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.OkButton", UiRect{ 1000, 894, 74, 22 }));
            layout.elements.push_back(MakeElement(L"CharacterCreate.CancelButton", UiRect{ 1100, 894, 74, 22 }));
        }
    }

    FrontendLayout FrontendLayoutCatalog::Create(const FrontendLayoutScreen screen)
    {
        return Create(screen, FrontendLayoutResolution::R1920x1080);
    }

    FrontendLayout FrontendLayoutCatalog::Create(const FrontendLayoutScreen screen, const FrontendLayoutResolution resolution)
    {
        FrontendLayout layout;
        layout.screen = screen;
        layout.resolution = resolution;
        switch (screen)
        {
        case FrontendLayoutScreen::Login:
            AddLoginElements(layout);
            break;
        case FrontendLayoutScreen::ServerSelect:
            AddServerSelectElements(layout);
            break;
        case FrontendLayoutScreen::CharacterSelect:
            AddCharacterSelectElements(layout);
            break;
        case FrontendLayoutScreen::CharacterCreate:
            AddCharacterCreateElements(layout);
            break;
        }

        if (resolution == FrontendLayoutResolution::R1024x768)
        {
            return ScaleToViewport(layout, ScreenViewport{ 1024, 768 });
        }

        return ScaleToViewport(layout, ScreenViewport{ 1920, 1080 });
    }

    FrontendLayoutResolution FrontendLayoutCatalog::SelectResolution(const std::uint32_t width, const std::uint32_t height) noexcept
    {
        if (width == 1024 && height == 768)
        {
            return FrontendLayoutResolution::R1024x768;
        }

        return FrontendLayoutResolution::R1920x1080;
    }

    FrontendLayout FrontendLayoutCatalog::ScaleToViewport(FrontendLayout layout, const ScreenViewport viewport)
    {
        const auto transform = LayoutViewport::BuildTransform(layout.referenceCanvas, viewport);
        layout.width = viewport.width;
        layout.height = viewport.height;
        for (auto& element : layout.elements)
        {
            element.rect = LayoutViewport::ScaleRect(element.rect, transform);
        }

        return layout;
    }
}
