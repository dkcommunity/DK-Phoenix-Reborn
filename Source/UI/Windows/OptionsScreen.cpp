#include "UI/Windows/OptionsScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

OptionsScreen::OptionsScreen() noexcept
    : isOpen_(false)
    , isTitleMode_(false)
    , selectedTab_(OptionTab::Control)
    , currentRace_(0)
    , acceleratorMode_(0)
    , focusedHotkey_(-1)
    , draggingSlider_(false)
    , draggedSlider_(ValueRect::MouseSpeed)
{
    // Initialize checks array
    checks_.fill(false);
    
    // Initialize slider values
    sliderValues_.fill(0);
    
    // Set defaults
    sliderValues_[static_cast<std::size_t>(ValueRect::MouseSpeed)] = 10;
    sliderValues_[static_cast<std::size_t>(ValueRect::Gamma)] = 50;
    sliderValues_[static_cast<std::size_t>(ValueRect::Sound)] = 80;
    sliderValues_[static_cast<std::size_t>(ValueRect::Music)] = 80;
    sliderValues_[static_cast<std::size_t>(ValueRect::Alpha)] = 15;
}

void OptionsScreen::Open(bool isTitleMode) noexcept
{
    isOpen_ = true;
    isTitleMode_ = isTitleMode;
    selectedTab_ = OptionTab::Control;
    controlScrollPos_ = 0;
    graphicScrollPos_ = 0;
    acceleratorMode_ = 0;
    focusedHotkey_ = -1;
    draggingSlider_ = false;

    InitializeSPKs();
    InitializeButtons();
    PositionButtons();
    LoadOptionValues();
    UpdateSelectedTabLayout();
}

void OptionsScreen::Close() noexcept
{
    isOpen_ = false;
    isTitleMode_ = false;
    mainSpk_.reset();
    etcSpk_.reset();
    scrollSpk_.reset();
    titleScrollSpk_.reset();
    tabButtonGroup_.reset();
    controlButtonGroup_.reset();
    graphicButtonGroup_.reset();
    soundButtonGroup_.reset();
    gameButtonGroup_.reset();
    otherButtonGroup_.reset();
    scrollBar_.reset();
    dialog_.reset();
}

void OptionsScreen::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open(false);
}

void OptionsScreen::LoadFromConfig(const settings::OptionsProfile& profile) noexcept
{
    config_ = profile;
}

void OptionsScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void OptionsScreen::InitializeSPKs() noexcept
{
    if (isTitleMode_)
    {
        mainSpk_ = std::make_unique<SpriteArchive>();
        if (!config_.titleOption1024SpkName.empty())
            mainSpk_->Open(config_.titleOption1024SpkName.c_str());
        else if (!config_.titleOptionSpkName.empty())
            mainSpk_->Open(config_.titleOptionSpkName.c_str());

        scrollSpk_ = std::make_unique<SpriteArchive>();
        if (!config_.titleScrollSpkName.empty())
            scrollSpk_->Open(config_.titleScrollSpkName.c_str());
    }
    else
    {
        // Game mode - race specific
        switch (currentRace_)
        {
            case 1: // Vampire
                mainSpk_ = std::make_unique<SpriteArchive>();
                if (!config_.vampireSpkName.empty())
                    mainSpk_->Open(config_.vampireSpkName.c_str());
                break;
            case 2: // Ousters
                mainSpk_ = std::make_unique<SpriteArchive>();
                if (!config_.oustersSpkName.empty())
                    mainSpk_->Open(config_.oustersSpkName.c_str());
                break;
            default: // Slayer
                mainSpk_ = std::make_unique<SpriteArchive>();
                if (!config_.slayerSpkName.empty())
                    mainSpk_->Open(config_.slayerSpkName.c_str());
                break;
        }

        etcSpk_ = std::make_unique<SpriteArchive>();
        if (!config_.etcSpkName.empty())
            etcSpk_->Open(config_.etcSpkName.c_str());

        scrollSpk_ = std::make_unique<SpriteArchive>();
        if (!config_.scrollSpkName.empty())
            scrollSpk_->Open(config_.scrollSpkName.c_str());
    }
}

void OptionsScreen::InitializeButtons() noexcept
{
    tabButtonGroup_ = std::make_unique<ButtonGroup>();

    if (isTitleMode_)
    {
        // Title mode tabs
        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Control),
            config_.tabControlX, config_.tabY,
            mainSpk_ ? mainSpk_->GetWidth(config_.tabControlHilightFrame) : 70,
            mainSpk_ ? mainSpk_->GetHeight(config_.tabControlHilightFrame) : 25,
            [this]() { OnTabSelected(OptionTab::Control); });

        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Graphic),
            config_.tabGraphicX, config_.tabY,
            mainSpk_ ? mainSpk_->GetWidth(config_.tabGraphicHilightFrame) : 70,
            mainSpk_ ? mainSpk_->GetHeight(config_.tabGraphicHilightFrame) : 25,
            [this]() { OnTabSelected(OptionTab::Graphic); });

        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Sound),
            config_.tabSoundX, config_.tabY,
            mainSpk_ ? mainSpk_->GetWidth(config_.tabSoundHilightFrame) : 70,
            mainSpk_ ? mainSpk_->GetHeight(config_.tabSoundHilightFrame) : 25,
            [this]() { OnTabSelected(OptionTab::Sound); });

        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Game),
            config_.tabGameX, config_.tabY,
            mainSpk_ ? mainSpk_->GetWidth(config_.tabGameHilightFrame) : 70,
            mainSpk_ ? mainSpk_->GetHeight(config_.tabGameHilightFrame) : 25,
            [this]() { OnTabSelected(OptionTab::Game); });

        // Other tab (1024+ resolution only)
        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Other),
            config_.tabOtherX, config_.tabY,
            mainSpk_ ? mainSpk_->GetWidth(config_.tabOtherHilightFrame) : 70,
            mainSpk_ ? mainSpk_->GetHeight(config_.tabOtherHilightFrame) : 25,
            [this]() { OnTabSelected(OptionTab::Other); });

        // Close button
        tabButtonGroup_->AddButton(99, // CLOSE_ID
            config_.titleCloseButtonX, config_.titleCloseButtonY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleButtonExitFrame) : 25,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleButtonExitFrame) : 25,
            [this]() { OnCloseClicked(); });

        // Reset button (Title mode)
        tabButtonGroup_->AddButton(100, // RESET_ID
            config_.titleResetButtonX, config_.titleResetButtonY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleButtonResetFrame) : 64,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleButtonResetFrame) : 24,
            [this]() { OnResetClicked(); });
    }
    else
    {
        // Game mode tabs
        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Control),
            config_.tabControlX, config_.tabY,
            etcSpk_ ? etcSpk_->GetWidth(config_.tabControlFrame) : 52,
            etcSpk_ ? etcSpk_->GetHeight(config_.tabControlFrame) : 22,
            [this]() { OnTabSelected(OptionTab::Control); });

        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Graphic),
            config_.tabGraphicX, config_.tabY,
            etcSpk_ ? etcSpk_->GetWidth(config_.tabGraphicFrame) : 52,
            etcSpk_ ? etcSpk_->GetHeight(config_.tabGraphicFrame) : 22,
            [this]() { OnTabSelected(OptionTab::Graphic); });

        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Sound),
            config_.tabSoundX, config_.tabY,
            etcSpk_ ? etcSpk_->GetWidth(config_.tabSoundFrame) : 52,
            etcSpk_ ? etcSpk_->GetHeight(config_.tabSoundFrame) : 22,
            [this]() { OnTabSelected(OptionTab::Sound); });

        tabButtonGroup_->AddButton(static_cast<int>(OptionTab::Game),
            config_.tabGameX, config_.tabY,
            etcSpk_ ? etcSpk_->GetWidth(config_.tabGameFrame) : 52,
            etcSpk_ ? etcSpk_->GetHeight(config_.tabGameFrame) : 22,
            [this]() { OnTabSelected(OptionTab::Game); });

        // Close button
        tabButtonGroup_->AddButton(99, // CLOSE_ID
            config_.closeButtonX, config_.closeButtonY,
            etcSpk_ ? etcSpk_->GetWidth(config_.closeButtonFrame) : 12,
            etcSpk_ ? etcSpk_->GetHeight(config_.closeButtonFrame) : 10,
            [this]() { OnCloseClicked(); });
    }

    // Control tab buttons
    controlButtonGroup_ = std::make_unique<ButtonGroup>();
    
    if (isTitleMode_)
    {
        // Radio buttons for chat style
        controlButtonGroup_->AddButton(static_cast<int>(CheckId::NormalChat),
            config_.checkX + 10, config_.checkY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleRadioBackFrame) : 16,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleRadioBackFrame) : 16,
            [this]() { OnCheckToggled(CheckId::NormalChat); });

        controlButtonGroup_->AddButton(static_cast<int>(CheckId::EnterChat),
            config_.checkX + 120, config_.checkY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleRadioBackFrame) : 16,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleRadioBackFrame) : 16,
            [this]() { OnCheckToggled(CheckId::EnterChat); });

        // Check boxes (starting from index 2 + CHECK_CONTROL_MAX)
        for (int i = 0; i < static_cast<int>(CheckId::CheckControlMax); ++i)
        {
            controlButtonGroup_->AddButton(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i,
                config_.checkX - 10, config_.checkY + config_.checkGap * (8 + i),
                mainSpk_ ? mainSpk_->GetWidth(config_.titleCheckBackFrame) : 16,
                mainSpk_ ? mainSpk_->GetHeight(config_.titleCheckBackFrame) : 16,
                [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i)); });
        }

        // Reset button (Title mode)
        int resetX = 10;
        int resetY = 10;
        if (mainSpk_)
        {
            resetX = config_.windowWidth - mainSpk_->GetWidth(config_.titleButtonResetFrame) - 40;
            resetY = config_.windowHeight - mainSpk_->GetHeight(config_.titleButtonResetFrame) - 20;
        }
        controlButtonGroup_->AddButton(101, // KEY_DEFAULT_INIT
            resetX, resetY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleButtonResetFrame) : 64,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleButtonResetFrame) : 24,
            [this]() { OnResetClicked(); });
    }
    else
    {
        // Game mode control tab
        // Radio buttons
        controlButtonGroup_->AddButton(static_cast<int>(CheckId::NormalChat),
            config_.checkX + 10, config_.checkY,
            etcSpk_ ? etcSpk_->GetWidth(config_.radioBackFrame) : 16,
            etcSpk_ ? etcSpk_->GetHeight(config_.radioBackFrame) : 16,
            [this]() { OnCheckToggled(CheckId::NormalChat); });

        controlButtonGroup_->AddButton(static_cast<int>(CheckId::EnterChat),
            config_.checkX + 120, config_.checkY,
            etcSpk_ ? etcSpk_->GetWidth(config_.radioBackFrame) : 16,
            etcSpk_ ? etcSpk_->GetHeight(config_.radioBackFrame) : 16,
            [this]() { OnCheckToggled(CheckId::EnterChat); });

        // Check boxes
        for (int i = 0; i < static_cast<int>(CheckId::CheckControlMax); ++i)
        {
            controlButtonGroup_->AddButton(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i,
                config_.checkX, config_.checkY + config_.checkGap * (8 + i),
                etcSpk_ ? etcSpk_->GetWidth(config_.checkBackFrame) : 16,
                etcSpk_ ? etcSpk_->GetHeight(config_.checkBackFrame) : 16,
                [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i)); });
        }

#if defined(ENABLE_UI_UPDATE)
        // Reset button (Game mode)
        controlButtonGroup_->AddButton(100, // RESET_ID
            10, 10, // Will be positioned
            etcSpk_ ? etcSpk_->GetWidth(config_.tabControlFrame) : 64,
            etcSpk_ ? etcSpk_->GetHeight(config_.tabControlFrame) : 24,
            [this]() { OnResetClicked(); });
#endif
    }

    // Graphic tab buttons
    graphicButtonGroup_ = std::make_unique<ButtonGroup>();

    if (isTitleMode_)
    {
        // Resolution radio buttons
        graphicButtonGroup_->AddButton(static_cast<int>(CheckId::Resolution800x600),
            config_.checkX + 10, config_.checkY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleRadioBackFrame) : 16,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleRadioBackFrame) : 16,
            [this]() { OnCheckToggled(CheckId::Resolution800x600); });

        graphicButtonGroup_->AddButton(static_cast<int>(CheckId::Resolution1024x768),
            config_.checkX + 120, config_.checkY,
            mainSpk_ ? mainSpk_->GetWidth(config_.titleRadioBackFrame) : 16,
            mainSpk_ ? mainSpk_->GetHeight(config_.titleRadioBackFrame) : 16,
            [this]() { OnCheckToggled(CheckId::Resolution1024x768); });

        for (int i = 0; i < static_cast<int>(CheckId::CheckGraphicMax) - static_cast<int>(CheckId::CheckGraphicTab); ++i)
        {
            graphicButtonGroup_->AddButton(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i,
                config_.checkX, config_.checkY + config_.checkGap * i,
                mainSpk_ ? mainSpk_->GetWidth(config_.titleCheckBackFrame) : 16,
                mainSpk_ ? mainSpk_->GetHeight(config_.titleCheckBackFrame) : 16,
                [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i)); });
        }
    }
    else
    {
        for (int i = 0; i < static_cast<int>(CheckId::CheckGraphicMax) - static_cast<int>(CheckId::CheckGraphicTab); ++i)
        {
            graphicButtonGroup_->AddButton(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i,
                config_.checkX, config_.checkY + config_.checkGap * i,
                etcSpk_ ? etcSpk_->GetWidth(config_.checkBackFrame) : 16,
                etcSpk_ ? etcSpk_->GetHeight(config_.checkBackFrame) : 16,
                [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckGraphicTab) + 2 + i)); });
        }
    }

    // Sound tab buttons
    soundButtonGroup_ = std::make_unique<ButtonGroup>();
    for (int i = 0; i < static_cast<int>(CheckId::CheckSoundMax) - static_cast<int>(CheckId::CheckSoundTab); ++i)
    {
        soundButtonGroup_->AddButton(static_cast<int>(CheckId::CheckSoundTab) + i,
            config_.checkX, config_.checkY + config_.checkGap * i,
            isTitleMode_ ? 
                (mainSpk_ ? mainSpk_->GetWidth(config_.titleCheckBackFrame) : 16) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.checkBackFrame) : 16),
            isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.titleCheckBackFrame) : 16) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.checkBackFrame) : 16),
            [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckSoundTab) + i)); });
    }

    // Game tab buttons
    gameButtonGroup_ = std::make_unique<ButtonGroup>();
    for (int i = 0; i < static_cast<int>(CheckId::CheckGameMax) - static_cast<int>(CheckId::CheckGameTab); ++i)
    {
        gameButtonGroup_->AddButton(static_cast<int>(CheckId::CheckGameTab) + i,
            config_.checkX, config_.checkY + config_.checkGap * i,
            isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetWidth(config_.titleCheckBackFrame) : 16) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.checkBackFrame) : 16),
            isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.titleCheckBackFrame) : 16) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.checkBackFrame) : 16),
            [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckGameTab) + i)); });
    }

    // Other tab buttons (Title mode, 1024+ resolution only)
    otherButtonGroup_ = std::make_unique<ButtonGroup>();
    if (isTitleMode_)
    {
        for (int i = 0; i < static_cast<int>(CheckId::CheckOtherMax) - static_cast<int>(CheckId::CheckOtherTab); ++i)
        {
            otherButtonGroup_->AddButton(static_cast<int>(CheckId::CheckOtherTab) + i,
                config_.checkX - 10, config_.checkY + config_.checkGap * i,
                mainSpk_ ? mainSpk_->GetWidth(config_.titleCheckBackFrame) : 16,
                mainSpk_ ? mainSpk_->GetHeight(config_.titleCheckBackFrame) : 16,
                [this, i]() { OnCheckToggled(static_cast<CheckId>(static_cast<int>(CheckId::CheckOtherTab) + i)); });
        }
    }

    // Scroll bar
    if (isTitleMode_)
    {
        scrollBar_ = std::make_unique<ScrollBar>(config_.titleScrollX, config_.titleScrollY,
            config_.titleScrollWidth, config_.titleScrollHeight,
            ScrollBar::Orientation::Vertical);
    }
    else
    {
        scrollBar_ = std::make_unique<ScrollBar>(config_.optionScrollX, config_.optionScrollY,
            config_.optionScrollWidth, config_.optionScrollHeight,
            ScrollBar::Orientation::Vertical);
    }
}

void OptionsScreen::PositionButtons() noexcept
{
    if (!tabButtonGroup_)
        return;

    tabButtonGroup_->SetPosition(static_cast<int>(OptionTab::Control), config_.tabControlX, config_.tabY);
    tabButtonGroup_->SetPosition(static_cast<int>(OptionTab::Graphic), config_.tabGraphicX, config_.tabY);
    tabButtonGroup_->SetPosition(static_cast<int>(OptionTab::Sound), config_.tabSoundX, config_.tabY);
    tabButtonGroup_->SetPosition(static_cast<int>(OptionTab::Game), config_.tabGameX, config_.tabY);
    tabButtonGroup_->SetPosition(static_cast<int>(OptionTab::Other), config_.tabOtherX, config_.tabY);

    if (isTitleMode_)
    {
        tabButtonGroup_->SetPosition(99, config_.titleCloseButtonX, config_.titleCloseButtonY);
        tabButtonGroup_->SetPosition(100, config_.titleResetButtonX, config_.titleResetButtonY);
    }
    else
    {
        tabButtonGroup_->SetPosition(99, config_.closeButtonX, config_.closeButtonY);
    }
}

void OptionsScreen::UpdateSelectedTabLayout() noexcept
{
    UpdateScrollRanges();
}

bool OptionsScreen::IsPointInWindow(int x, int y) const noexcept
{
    return x >= config_.windowX && x < config_.windowX + config_.windowWidth &&
           y >= config_.windowY && y < config_.windowY + config_.windowHeight;
}

bool OptionsScreen::IsPointInTab(int x, int y, OptionTab tab) const noexcept
{
    int tabX = 0, tabY = config_.tabY;
    int tabW = 0, tabH = 0;

    switch (tab)
    {
        case OptionTab::Control:
            tabX = config_.tabControlX;
            tabW = isTitleMode_ ? 
                (mainSpk_ ? mainSpk_->GetWidth(config_.tabControlHilightFrame) : 70) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.tabControlFrame) : 52);
            tabH = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.tabControlHilightFrame) : 25) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.tabControlFrame) : 22);
            break;
        case OptionTab::Graphic:
            tabX = config_.tabGraphicX;
            tabW = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetWidth(config_.tabGraphicHilightFrame) : 70) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.tabGraphicFrame) : 52);
            tabH = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.tabGraphicHilightFrame) : 25) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.tabGraphicFrame) : 22);
            break;
        case OptionTab::Sound:
            tabX = config_.tabSoundX;
            tabW = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetWidth(config_.tabSoundHilightFrame) : 70) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.tabSoundFrame) : 52);
            tabH = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.tabSoundHilightFrame) : 25) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.tabSoundFrame) : 22);
            break;
        case OptionTab::Game:
            tabX = config_.tabGameX;
            tabW = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetWidth(config_.tabGameHilightFrame) : 70) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.tabGameFrame) : 52);
            tabH = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.tabGameHilightFrame) : 25) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.tabGameFrame) : 22);
            break;
        case OptionTab::Other:
            tabX = config_.tabOtherX;
            tabW = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetWidth(config_.tabOtherHilightFrame) : 70) :
                (etcSpk_ ? etcSpk_->GetWidth(config_.tabOtherFrame) : 52);
            tabH = isTitleMode_ ?
                (mainSpk_ ? mainSpk_->GetHeight(config_.tabOtherHilightFrame) : 25) :
                (etcSpk_ ? etcSpk_->GetHeight(config_.tabOtherFrame) : 22);
            break;
    }

    return x >= tabX && x < tabX + tabW &&
           y >= tabY && y < tabY + tabH;
}

bool OptionsScreen::IsPointInSlider(int x, int y, ValueRect rect) const noexcept
{
    // Simplified hit test for slider areas
    int sliderX = 0, sliderY = 0, sliderW = 0, sliderH = 0;

    switch (rect)
    {
        case ValueRect::MouseSpeed:
            sliderX = config_.mouseSpeedValueX;
            sliderY = config_.mouseSpeedValueY;
            sliderW = config_.mouseSpeedValueWidth;
            sliderH = config_.mouseSpeedValueHeight;
            break;
        case ValueRect::Gamma:
            sliderX = config_.gammaValueX;
            sliderY = config_.gammaValueY;
            sliderW = config_.gammaValueWidth;
            sliderH = config_.gammaValueHeight;
            break;
        case ValueRect::Sound:
            sliderX = config_.soundValueX;
            sliderY = config_.soundValueY;
            sliderW = config_.soundValueWidth;
            sliderH = config_.soundValueHeight;
            break;
        case ValueRect::Music:
            sliderX = config_.musicValueX;
            sliderY = config_.musicValueY;
            sliderW = config_.musicValueWidth;
            sliderH = config_.musicValueHeight;
            break;
        case ValueRect::Alpha:
            sliderX = config_.alphaValueX;
            sliderY = config_.alphaValueY;
            sliderW = config_.alphaValueWidth;
            sliderH = config_.alphaValueHeight;
            break;
    }

    return x >= sliderX && x < sliderX + sliderW &&
           y >= sliderY && y < sliderY + sliderH;
}

bool OptionsScreen::IsPointInCheck(int x, int y, CheckId id) const noexcept
{
    // Simplified - check if point is in check box area
    int checkBaseX = config_.checkX;
    int checkBaseY = config_.checkY;
    int checkGap = config_.checkGap;

    int index = static_cast<int>(id);
    int row = 0;

    if (index < static_cast<int>(CheckId::CheckControlMax))
    {
        row = index;
    }
    else if (index < static_cast<int>(CheckId::CheckGraphicMax))
    {
        row = index - static_cast<int>(CheckId::CheckGraphicTab) - 2;
    }
    else if (index < static_cast<int>(CheckId::CheckSoundMax))
    {
        row = index - static_cast<int>(CheckId::CheckSoundTab);
    }
    else if (index < static_cast<int>(CheckId::CheckGameMax))
    {
        row = index - static_cast<int>(CheckId::CheckGameTab);
    }
    else if (index < static_cast<int>(CheckId::CheckOtherMax))
    {
        row = index - static_cast<int>(CheckId::CheckOtherTab);
    }

    int checkX = checkBaseX;
    int checkY = checkBaseY + checkGap * row;
    int checkW = isTitleMode_ ? 
        (mainSpk_ ? mainSpk_->GetWidth(config_.titleCheckBackFrame) : 16) :
        (etcSpk_ ? etcSpk_->GetWidth(config_.checkBackFrame) : 16);
    int checkH = isTitleMode_ ?
        (mainSpk_ ? mainSpk_->GetHeight(config_.titleCheckBackFrame) : 16) :
        (etcSpk_ ? etcSpk_->GetHeight(config_.checkBackFrame) : 16);

    return x >= checkX && x < checkX + checkW &&
           y >= checkY && y < checkY + checkH;
}

bool OptionsScreen::IsPointInResetButton(int x, int y) const noexcept
{
    int resetX = isTitleMode_ ? config_.titleResetButtonX : config_.windowWidth / 2 - 32;
    int resetY = isTitleMode_ ? config_.titleResetButtonY : config_.windowHeight - 40;
    int resetW = isTitleMode_ ?
        (mainSpk_ ? mainSpk_->GetWidth(config_.titleButtonResetFrame) : 64) :
        (etcSpk_ ? etcSpk_->GetWidth(config_.tabControlFrame) : 64);
    int resetH = isTitleMode_ ?
        (mainSpk_ ? mainSpk_->GetHeight(config_.titleButtonResetFrame) : 24) :
        (etcSpk_ ? etcSpk_->GetHeight(config_.tabControlFrame) : 24);

    return x >= resetX && x < resetX + resetW &&
           y >= resetY && y < resetY + resetH;
}

void OptionsScreen::OnTabSelected(OptionTab tab) noexcept
{
    selectedTab_ = tab;
    UpdateSelectedTabLayout();
}

void OptionsScreen::OnCloseClicked() noexcept
{
    if (closeCallback_)
        closeCallback_();
    Close();
}

void OptionsScreen::OnResetClicked() noexcept
{
    // Show confirmation dialog
    dialog_ = std::make_unique<DialogWindow>(DialogWindow::DialogType::ExitConfirm);
    dialog_->Show();
}

void OptionsScreen::OnCheckToggled(CheckId id) noexcept
{
    checks_[static_cast<std::size_t>(id)] = !checks_[static_cast<std::size_t>(id)];
    
    if (optionChangeCallback_)
        optionChangeCallback_(id, checks_[static_cast<std::size_t>(id)]);
}

void OptionsScreen::OnSliderDragged(ValueRect rect, std::int32_t value) noexcept
{
    sliderValues_[static_cast<std::size_t>(rect)] = value;
    
    if (sliderChangeCallback_)
        sliderChangeCallback_(rect, value);
}

void OptionsScreen::OnScroll(int delta) noexcept
{
    if (delta > 0)
    {
        switch (selectedTab_)
        {
            case OptionTab::Control:
                if (controlScrollPos_ > 0)
                    controlScrollPos_--;
                break;
            case OptionTab::Graphic:
                if (graphicScrollPos_ > 0)
                    graphicScrollPos_--;
                break;
        }
    }
    else
    {
        switch (selectedTab_)
        {
            case OptionTab::Control:
                if (controlScrollPos_ < 10)
                    controlScrollPos_++;
                break;
            case OptionTab::Graphic:
                if (graphicScrollPos_ < 10)
                    graphicScrollPos_++;
                break;
        }
    }
}

void OptionsScreen::UpdateScrollRanges() noexcept
{
    if (scrollBar_)
    {
        int maxScroll = 0;
        switch (selectedTab_)
        {
            case OptionTab::Control:
                maxScroll = 10; // Approximate
                break;
            case OptionTab::Graphic:
                maxScroll = 10;
                break;
        }
        scrollBar_->SetPosMax(maxScroll);
    }
}

void OptionsScreen::LoadOptionValues() noexcept
{
    // Load from config/profile
    // This would load from g_pUserOption in legacy
}

void OptionsScreen::SaveOptionValues() noexcept
{
    // Save to config/profile
}

void OptionsScreen::ApplyResolution() noexcept
{
    // Legacy: g_pUserOption->Resolution1024
}

void OptionsScreen::ApplyGamma() noexcept
{
    // Legacy: CDirectDraw::SetGammaRamp
}

void OptionsScreen::ApplyVSync() noexcept
{
    // Legacy: Phoenix_D3D9_ApplyVSyncSetting
}

void OptionsScreen::ApplyVignette() noexcept
{
    // Legacy: D3D9PostProcess_SetVignetteEnabled
}

void OptionsScreen::ApplyCrtScanline() noexcept
{
    // Legacy: D3D9PostProcess_SetScanlineEnabled
}

bool OptionsScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check tabs
    for (int i = 0; i < static_cast<int>(OptionTab::Count); ++i)
    {
        OptionTab tab = static_cast<OptionTab>(i);
        if (IsPointInTab(x, y, tab))
        {
            OnTabSelected(tab);
            return true;
        }
    }

    // Check sliders
    for (int i = 0; i < static_cast<int>(ValueRect::Max); ++i)
    {
        ValueRect rect = static_cast<ValueRect>(i);
        if (IsPointInSlider(x, y, rect))
        {
            draggingSlider_ = true;
            draggedSlider_ = rect;
            return true;
        }
    }

    // Check close button
    if (isTitleMode_)
    {
        int closeX = config_.titleCloseButtonX;
        int closeY = config_.titleCloseButtonY;
        int closeW = mainSpk_ ? mainSpk_->GetWidth(config_.titleButtonExitFrame) : 25;
        int closeH = mainSpk_ ? mainSpk_->GetHeight(config_.titleButtonExitFrame) : 25;
        if (x >= closeX && x < closeX + closeW &&
            y >= closeY && y < closeY + closeH)
        {
            OnCloseClicked();
            return true;
        }
    }
    else
    {
        int closeX = config_.closeButtonX;
        int closeY = config_.closeButtonY;
        int closeW = etcSpk_ ? etcSpk_->GetWidth(config_.closeButtonFrame) : 12;
        int closeH = etcSpk_ ? etcSpk_->GetHeight(config_.closeButtonFrame) : 10;
        if (x >= closeX && x < closeX + closeW &&
            y >= closeY && y < closeY + closeH)
        {
            OnCloseClicked();
            return true;
        }
    }

    // Check reset button
    if (IsPointInResetButton(x, y))
    {
        OnResetClicked();
        return true;
    }

    // Check tab buttons
    if (tabButtonGroup_)
    {
        if (tabButtonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check control tab buttons
    if (selectedTab_ == OptionTab::Control && controlButtonGroup_)
    {
        if (controlButtonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check graphic tab buttons
    if (selectedTab_ == OptionTab::Graphic && graphicButtonGroup_)
    {
        if (graphicButtonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check sound tab buttons
    if (selectedTab_ == OptionTab::Sound && soundButtonGroup_)
    {
        if (soundButtonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check game tab buttons
    if (selectedTab_ == OptionTab::Game && gameButtonGroup_)
    {
        if (gameButtonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check other tab buttons
    if (selectedTab_ == OptionTab::Other && otherButtonGroup_)
    {
        if (otherButtonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check scroll bar
    if (scrollBar_)
    {
        scrollBar_->HandleMouseDown(x, y);
        return true;
    }

    // Check sliders
    for (int i = 0; i < static_cast<int>(ValueRect::Max); ++i)
    {
        ValueRect rect = static_cast<ValueRect>(i);
        if (IsPointInSlider(x, y, rect))
        {
            draggingSlider_ = true;
            draggedSlider_ = rect;
            OnSliderDragged(rect, x);
            return true;
        }
    }

    // Check dialog
    if (dialog_)
    {
        dialog_->HandleMouseDown(x, y, rightButton);
        return true;
    }

    return false;
}

bool OptionsScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    draggingSlider_ = false;

    if (tabButtonGroup_)
    {
        tabButtonGroup_->HandleMouseUp(x, y);
    }

    if (controlButtonGroup_)
    {
        controlButtonGroup_->HandleMouseUp(x, y);
    }

    if (graphicButtonGroup_)
    {
        graphicButtonGroup_->HandleMouseUp(x, y);
    }

    if (soundButtonGroup_)
    {
        soundButtonGroup_->HandleMouseUp(x, y);
    }

    if (gameButtonGroup_)
    {
        gameButtonGroup_->HandleMouseUp(x, y);
    }

    if (otherButtonGroup_)
    {
        otherButtonGroup_->HandleMouseUp(x, y);
    }

    if (scrollBar_)
    {
        scrollBar_->HandleMouseUp(x, y);
    }

    if (dialog_)
    {
        dialog_->HandleMouseUp(x, y, rightButton);
    }

    return false;
}

bool OptionsScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (draggingSlider_)
    {
        OnSliderDragged(draggedSlider_, x);
        return true;
    }

    if (tabButtonGroup_)
    {
        tabButtonGroup_->HandleMouseMove(x, y);
    }

    if (controlButtonGroup_)
    {
        controlButtonGroup_->HandleMouseMove(x, y);
    }

    if (graphicButtonGroup_)
    {
        graphicButtonGroup_->HandleMouseMove(x, y);
    }

    if (soundButtonGroup_)
    {
        soundButtonGroup_->HandleMouseMove(x, y);
    }

    if (gameButtonGroup_)
    {
        gameButtonGroup_->HandleMouseMove(x, y);
    }

    if (otherButtonGroup_)
    {
        otherButtonGroup_->HandleMouseMove(x, y);
    }

    if (scrollBar_)
    {
        scrollBar_->HandleMouseMove(x, y);
    }

    if (dialog_)
    {
        dialog_->HandleMouseDown(x, y, false);
    }

    return false;
}

bool OptionsScreen::HandleMouseWheel(int delta) noexcept
{
    if (!isOpen_)
        return false;

    OnScroll(delta);
    return true;
}

bool OptionsScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        OnCloseClicked();
        return true;
    }

    if (key == VK_F5)
    {
        ReloadConfig();
        return true;
    }

    // Tab navigation
    if (key == VK_TAB)
    {
        int currentTab = static_cast<int>(selectedTab_);
        currentTab = (currentTab + 1) % static_cast<int>(OptionTab::Count);
        // Skip Other tab if not in title mode or not high resolution
        if (currentTab == static_cast<int>(OptionTab::Other) && (!isTitleMode_ || config_.windowWidth <= 800))
        {
            currentTab = 0;
        }
        OnTabSelected(static_cast<OptionTab>(currentTab));
        return true;
    }

    return false;
}

void OptionsScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (tabButtonGroup_)
    {
        tabButtonGroup_->Update(deltaTime);
    }

    if (controlButtonGroup_)
    {
        controlButtonGroup_->Update(deltaTime);
    }

    if (graphicButtonGroup_)
    {
        graphicButtonGroup_->Update(deltaTime);
    }

    if (soundButtonGroup_)
    {
        soundButtonGroup_->Update(deltaTime);
    }

    if (gameButtonGroup_)
    {
        gameButtonGroup_->Update(deltaTime);
    }

    if (otherButtonGroup_)
    {
        otherButtonGroup_->Update(deltaTime);
    }

    if (scrollBar_)
    {
        scrollBar_->Update(deltaTime);
    }

    if (dialog_)
    {
        dialog_->Update(deltaTime);
    }
}

void OptionsScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    // Render background
    if (mainSpk_)
    {
        mainSpk_->Draw(config_.windowX, config_.windowY, 0);
    }

    // Render tab buttons
    if (tabButtonGroup_)
    {
        tabButtonGroup_->Render();
    }

    // Render selected tab content
    switch (selectedTab_)
    {
        case OptionTab::Control:
            if (controlButtonGroup_)
            {
                controlButtonGroup_->Render();
            }
            if (scrollBar_)
            {
                if (isTitleMode_)
                    scrollBar_->Render(); // Show3 for title mode
                else
                    scrollBar_->Render();
            }
            // Render accelerator/hotkey list
            break;

        case OptionTab::Graphic:
            if (graphicButtonGroup_)
            {
                graphicButtonGroup_->Render();
            }
            if (scrollBar_)
            {
                if (isTitleMode_)
                    scrollBar_->Render();
                else
                    scrollBar_->Render();
            }
            // Render gamma/alpha sliders
            break;

        case OptionTab::Sound:
            if (soundButtonGroup_)
            {
                soundButtonGroup_->Render();
            }
            // Render volume sliders
            break;

        case OptionTab::Game:
            if (gameButtonGroup_)
            {
                gameButtonGroup_->Render();
            }
            break;

        case OptionTab::Other:
            if (otherButtonGroup_)
            {
                otherButtonGroup_->Render();
            }
            break;
    }

    // Render dialog
    if (dialog_)
    {
        dialog_->Render();
    }

    // Render scroll bar
    if (scrollBar_)
    {
        if (isTitleMode_)
            scrollBar_->Render(); // Show3 for title mode
        else
            scrollBar_->Render();
    }
}

} // namespace phoenix::ui